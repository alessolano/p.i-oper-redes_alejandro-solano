#ifndef FS_H
#define FS_H

#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <ctime>

#define DIRECTORY_SIZE 3  // maximo de inodos(archivos) por directorio
#define TOTAL_BLOCKS 1000  //  numero total de bloques en la diskFile
#define BLOCK_SIZE 512  //  inodeSize en bytes de cada bloque
#define DIRECT_BLOCK_SIZE 4  //  inodeSize bloques directos
#define INDIRECT_BLOCK_SIZE 2 //  inodeSize de arreglo de bloques indirectos
#define MAX_NAME_LENGTH 64  //  inodeSize maximo del name
#define MAX_DATE_LENGTH 20  //  inodeSize maximo de fecha

typedef struct superBlock {
  int TotalBlocks;  //  numero total de bloques en la diskFile
  int blockSize;  //  inodeSize en bytes de cada bloque
  int freeBlocks;  //  bloques disponibles para guardar informacion
  int maxInodes;  //  maximo de inodos(archivos) en el sistema
  int usedInodes;  //  inodos en uso
  int firstFreeBlock;
};

typedef struct inode{
  char name[MAX_NAME_LENGTH];  //  name del archivo
  char date[MAX_DATE_LENGTH];  //  fecha de creacion
  int inodeSize;  //  inodeSize actual del archivo
  bool active = false;

  int directBlocks[DIRECT_BLOCK_SIZE];  //  arreglo de bloques directos
  int indirectBlocks[INDIRECT_BLOCK_SIZE];  //  arreglo de bloques indirectos
};

class FS {
 public:
  FS();
  ~FS();

  //  crea un inode vacio sin asignarle bloques
  int create(const std::string& name);
  //  add contenido a un inode vacio
  int add(const std::string& name, const std::string& data);
  //  Deletes a file
  int deleteFile(const std::string& fileName);
  //  imprimir los metadatos de un inode
  void printInode(const std::string& name);
  //  imprimir metadatos del super bloque
  void printSB();
  //  imprimir el contenido de un inode
  int printInodeContent(std::string name);

  int changeName(std::string name,std::string newName);


  //
  void fileList();

 private:
  std::fstream diskFile;  //  archivo que simula la diskFile de almacenamiento
  superBlock sb;  //  superBlock del sistema de archivos
  std::vector<inode> inodesTable;  //  tabla de archivos (inodos)
  int sizeTablaBytes;  //  inodeSize en bytes de inodesTable
  int tableBlocks;  //  cuantos bloques usa la tabla de inodos
  int bitMapBlocks;
  int sizeBitMapBytes;
  int superBlockBlocks;
  std::vector<int> bitMap;  //  mapa de bits para gestionar bloques

  //  buscar un inode y retornar su indice o -1 si no existe
  int searchInode(const std::string& name);
  //  buscar los bloques libres necesarios en el bitmap y retornar un arreglo con sus direcciones 
  std::vector<int> findFreeBlock(int cantidad);
  //  escribir contenido en la diskFile
  int writeDisk(inode& node, const std::string& data, int blocksNeeded, std::vector<int>& blocks);
  //  actualizar la diskFile
  void saveChanges();
  //  free data blocks
  int freeDataBlocks(const inode& node);
  std::string getActualDate();

};

#endif  //  FS_H
