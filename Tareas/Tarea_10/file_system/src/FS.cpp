#include "../include/FS.h"
#include <iostream>

FS::FS() {
  this->diskFile.open("diskFile.bin", std::ios::in | std::ios::out | std::ios::binary);
  if (!this->diskFile.is_open()) {
    this->diskFile.open("diskFile.bin", std::ios::out | std::ios::binary);
    this->diskFile.close();
    this->diskFile.open("diskFile.bin", std::ios::in | std::ios::out | std::ios::binary);

    if (!this->diskFile.is_open()) {
      std::cout << "Could not create diskFile" << std::endl;
      exit(1);
    }

  }

  sb.TotalBlocks = TOTAL_BLOCKS;
  sb.blockSize = BLOCK_SIZE;
  sb.freeBlocks = TOTAL_BLOCKS;
  sb.maxInodes = DIRECTORY_SIZE;
  sb.usedInodes = 0;

  inodesTable.resize(sb.maxInodes);  //  tabla de inodeSize maximo de inodos
  bitMap.resize(sb.TotalBlocks, 0); // 0 = libre, 1 = ocupado

  //  super bloque blocks
  this->superBlockBlocks = 1;
  //  bitmap blocks
  this->sizeBitMapBytes = sizeof(int) * sb.TotalBlocks;
  this->bitMapBlocks = (this->sizeBitMapBytes + BLOCK_SIZE - 1) / BLOCK_SIZE;
  //  tabla de inodos blocks
  this->sizeTablaBytes = sizeof(inode) * inodesTable.size();
  this->tableBlocks = (this->sizeTablaBytes + BLOCK_SIZE - 1) / BLOCK_SIZE;

  int actualBlock = 0;
  
  bitMap[actualBlock++] = 1;  //  espacio para el superBlock

  bitMap[1] = 1;  //  espacio para el bitmap

  for (int i = 0; i < this->bitMapBlocks; i++) {
    bitMap[actualBlock++] = 1;
  }

  for (int i = 0; i < tableBlocks; i++) {
    bitMap[actualBlock++] = 1;  //  se marcan los blocks usados por la tabla de inodos
  }

  int systemBlocks = this->superBlockBlocks + this->bitMapBlocks + this->tableBlocks;

  sb.freeBlocks -= systemBlocks;

  this->sb.firstFreeBlock = systemBlocks;

  //  asegurar que el archivo tenga el inodeSize correcto
  this->diskFile.seekp(TOTAL_BLOCKS * BLOCK_SIZE - 1);
  this->diskFile.write("", 1);
  this->diskFile.flush();

  saveChanges();
}

FS::~FS() {
  this->diskFile.close();
}

int FS::create(const std::string& name) {
  if (this->searchInode(name) != -1 || this->sb.maxInodes <= sb.usedInodes){ 
    throw std::runtime_error("FS::create failed");
  }

  int freeNode = -1;
  for (int i = 0; i < sb.maxInodes; i++) {
    if (!inodesTable[i].active) {
      freeNode = i;
      break;
    }
  }

  inode newInode = this->inodesTable[freeNode];
  memset(&newInode, 0, sizeof(inode));
  strncpy(newInode.name, name.c_str(), MAX_NAME_LENGTH - 1);
  std::string actualDate = getActualDate();
  strncpy(newInode.date, actualDate.c_str(), MAX_DATE_LENGTH - 1);
  newInode.inodeSize = 0;
  newInode.active = true;

  for (int i = 0; i < DIRECT_BLOCK_SIZE; i++) {
    newInode.directBlocks[i] = -1;
  }

  for (int i = 0; i < INDIRECT_BLOCK_SIZE; i++) {
    newInode.indirectBlocks[i] = -1;
  }

  this->inodesTable[this->sb.usedInodes++] = newInode;

  saveChanges();

  return 0;
}

int FS::add(const std::string &name, const std::string& data) {
  int index = -1;
  int blocksNeeded;

  // searchInode inode con ese name
  index = this->searchInode(name);
  if (index == -1) {
    std::cout << "File \"" << name << "\" does not exist." << std::endl;
    return -1;
  }

  // Obtener el inode
  inode& node = inodesTable[index];

  node.inodeSize = data.size();
  blocksNeeded = (int)((node.inodeSize + this->sb.blockSize -1) / this->sb.blockSize);
  if (blocksNeeded > this->sb.freeBlocks) {
    std::cout << "Insufficient space to store this file" << std::endl;
    return -1;
  }

  std::vector<int> blocks = findFreeBlock(blocksNeeded);

  for (int i = 0; i < blocks.size(); i++) {
    if (i < DIRECT_BLOCK_SIZE) {
      node.directBlocks[i] = blocks[i];
    } else {
      node.indirectBlocks[i - DIRECT_BLOCK_SIZE] = blocks[i];
    }
  }

  if (writeDisk(node, data, blocksNeeded, blocks) == -1) {
    std::cerr << "Error al escribir datos en disco\n";
    return -1;
  }

  this->sb.freeBlocks -= blocksNeeded;
  saveChanges();

  return 0;
}

int FS::deleteFile(const std::string& name) {
  int index = this->searchInode(name);
  if (index == -1) {
    std::cout << "The file \"" << name << "\" does not exist en the system." << std::endl;
    return -1;
  }

  inode& node = this->inodesTable[index];

  freeDataBlocks(node);

  node.active = false;

  this->sb.usedInodes--;

  this->saveChanges();
  std::cout << "Archivo \"" << name << "\" eliminado exitosamente." << std::endl;

  return 0;
}

void FS::printInode(const std::string& name) {
  int index = searchInode(name);
  if (index == -1) {
    std::cout << "El archivo \"" << name << "\" no existe en el sistema." << std::endl;
  }

  // Obtener el inode
  const inode& node = inodesTable[index];

  // Imprimir metadata
  std::cout << "=== inode del archivo: " << node.name << " ===" << std::endl;
  std::cout << "date: " << node.date << std::endl;
  std::cout << "inodeSize: " << node.inodeSize << " bytes" << std::endl;

  // blocks directos
  std::cout << "blocks directos: ";
  for (int i = 0; i < DIRECT_BLOCK_SIZE; i++) {
    if (node.directBlocks[i] != -1){
      std::cout << node.directBlocks[i] << " ";
    }
  }
  std::cout << std::endl;

  // blocks indirectos
  std::cout << "blocks indirectos: ";
  for (int i = 0; i < INDIRECT_BLOCK_SIZE; i++) {
    if (node.indirectBlocks[i] != -1){
      std::cout << node.indirectBlocks[i] << " ";
    }
  }
  std::cout << std::endl;
}

void FS::printSB() {
  std::cout << "superBlock:" << std::endl;
  std::cout << "Total blocks: " << sb.TotalBlocks << std::endl;
  std::cout << "inodeSize bloque: " << sb.blockSize << std::endl;
  std::cout << "blocks libres: " << sb.freeBlocks << std::endl;
  std::cout << "Maximo inodos: " << sb.maxInodes << std::endl;
  std::cout << "Inodos usados: " << sb.usedInodes << std::endl;
}

int FS::printInodeContent(std::string name) {
  int index = this->searchInode(name);
  if (index == -1) {
    std::cout << "El archivo \"" << name << "\" no existe en el sistema." << std::endl;
    return 0;
  }

  const inode& node = this->inodesTable[index];
  if(node.inodeSize == 0) {
    std::cout << "El archivo \"" << name << "\" no contiene datos." << std::endl;
    return 0;
  }

  int tableBlocks = (sizeTablaBytes + BLOCK_SIZE - 1) / BLOCK_SIZE;
  int offsetInicioDatos = (2 + tableBlocks) * BLOCK_SIZE;

  std::cout << node.name << " :"<< std::endl;

  size_t bytesLeidos = 0;
  std::vector<char> buffer(BLOCK_SIZE);

  //  blocks directos
  for (int i = 0; i < DIRECT_BLOCK_SIZE && bytesLeidos < node.inodeSize; i++) {

    if (node.directBlocks[i] == -1){
      continue;
    }

    this->diskFile.seekg(offsetInicioDatos + node.directBlocks[i] * BLOCK_SIZE);
    this->diskFile.read(buffer.data(), BLOCK_SIZE);

    size_t bytesRestantes = std::min((size_t)BLOCK_SIZE, node.inodeSize - bytesLeidos);
    std::cout.write(buffer.data(), bytesRestantes);
    bytesLeidos += bytesRestantes;

  }

  //  blocks indirectos
  for (int i = 0; i < INDIRECT_BLOCK_SIZE && bytesLeidos < node.inodeSize; i++) {
    if (node.indirectBlocks[i] == -1){
      continue;
    }

    this->diskFile.seekg(offsetInicioDatos + node.indirectBlocks[i] * BLOCK_SIZE);
    this->diskFile.read(buffer.data(), BLOCK_SIZE);

    size_t porLeer = std::min((size_t)BLOCK_SIZE, node.inodeSize - bytesLeidos);
    std::cout.write(buffer.data(), porLeer);
    bytesLeidos += porLeer;
  }

  std::cout << std::endl;
  return 0;
}

int FS::changeName(std::string name,std::string newName) {
  int index = this->searchInode(name);
  if (index == -1) {
    std::cout << "File \"" << name << "\" does not exist." << std::endl;
    return -1;
  }

  inode& node = inodesTable[index];

  strncpy(node.name, newName.c_str(), MAX_NAME_LENGTH - 1);
  node.name[MAX_NAME_LENGTH - 1] = '\0';

  this->saveChanges();

  return 0;

}

void FS::fileList() {
  std::cout << "=== Lista de Archivos ===" << std::endl;
    std::cout << std::left << std::setw(20) << "name" 
              << std::setw(12) << "date" 
              << std::setw(10) << "Tamaño" 
              << std::setw(8) << "Estado" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    bool hayArchivos = false;
    for (int i = 0; i < sb.maxInodes; i++) {
        if (inodesTable[i].active) {
            hayArchivos = true;
            std::cout << std::left << std::setw(20) << inodesTable[i].name
                      << std::setw(12) << inodesTable[i].date
                      << std::setw(10) << inodesTable[i].inodeSize
                      << std::setw(8) << "Activo" << std::endl;
        }
    }
    
    if (!hayArchivos) {
        std::cout << "No hay archivos en el sistema." << std::endl;
    }
}
int FS::searchInode(const std::string &name)
{
  for (int i = 0; i < sb.usedInodes; i++) {
    if (std::string (this->inodesTable[i].name) == name) {
      return i;
    }
  }
  return -1;
}

std::vector<int> FS::findFreeBlock(int cantidad) {
  int encontrados = 0;
  std::vector<int> blocks;
  for (int i = 0; i < this->sb.TotalBlocks; i++) {
    if (bitMap[i] == 0) {
      bitMap[i] = 1;
      encontrados++;
      blocks.push_back(i);
      if (encontrados == cantidad){
        return blocks;
      }
    }
  }

  throw std::runtime_error("FS::findFreeBlock failed");
}

int FS::writeDisk(inode& node, const std::string& data, int blocksNeeded, std::vector<int>& blocks) {

  for (int i = 0; i < blocksNeeded; i++) {
    int indiceBloque = blocks[i];
    size_t offset = i * this->sb.blockSize;

    size_t bytesRestantes = std::min((size_t)sb.blockSize, data.size() - offset);

    std::vector<char> buffer(sb.blockSize, 0);

    if(bytesRestantes > 0) {
      memcpy(buffer.data(), data.data() + offset, bytesRestantes);
    }

    int offsetInicioDatos = (2 + tableBlocks) * BLOCK_SIZE;
    this->diskFile.seekp(offsetInicioDatos + indiceBloque * this->sb.blockSize);
    this->diskFile.write(buffer.data(), this->sb.blockSize);


    if(!diskFile) {
      std::cout << "No se pudo escribir en el disco\n";
      return -1;
    }
  }

  this->diskFile.flush();

  return 0;
}

void FS::saveChanges() {
  //  escribir super bloque en bloque 0
  this->diskFile.seekp(0 * BLOCK_SIZE);
  this->diskFile.write(reinterpret_cast<char*>(&sb), sizeof(this->sb));
  this->diskFile.flush();
  //  escribir bitMap en bloque 1
  this->diskFile.seekp(1 * BLOCK_SIZE);
  this->diskFile.write(reinterpret_cast<char*>(bitMap.data()), sizeof(int) * bitMap.size());
  this->diskFile.flush();

  //  escribir la tabla de inodos en bloque 2 en adelante
  this->diskFile.seekp(2 * BLOCK_SIZE);
  this->diskFile.write(reinterpret_cast<char*>(inodesTable.data()), this->sizeTablaBytes);
  this->diskFile.flush();
}

int FS::freeDataBlocks(const inode& node){
  //  free direct blocks
  for (int i = 0; i < DIRECT_BLOCK_SIZE; i++) {
    if (node.directBlocks[i] == 0) {
      this->bitMap[node.directBlocks[i]] = 0;
      this->sb.freeBlocks++;
    }
  }

  //  free indirect blocks  
  for (int i = 0; i < INDIRECT_BLOCK_SIZE; i++) {
    if (node.indirectBlocks[i] == 0) {
      this->bitMap[node.indirectBlocks[i]] = 0;
      this->sb.freeBlocks++;
    }
  }
  return 0;
}

std::string FS::getActualDate() {
  time_t now = time(0);
  tm* ltm = localtime(&now);
    
  char buffer[MAX_DATE_LENGTH];
  snprintf(buffer, MAX_DATE_LENGTH, "%04d-%02d-%02d", 
          1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday);
  return std::string(buffer);
}
