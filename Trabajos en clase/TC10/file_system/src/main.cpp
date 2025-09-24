#include <iostream>
#include "../include/FS.h"

void showMenu() {
    std::cout << "\n=== SISTEMA DE ARCHIVOS ===\n";
    std::cout << "1. Crear archivo\n";
    std::cout << "2. Agregar contenido\n";
    std::cout << "3. Mostrar contenido\n";
    std::cout << "4. Listar archivos\n";
    std::cout << "5. Eliminar archivo\n";
    std::cout << "6. Mostrar superbloque\n";
    std::cout << "7. cambiar nombre de archivo\n";
    std::cout << "0. Salir\n";
    std::cout << "Opción: ";
}

int main() {
  FS* fs = new FS();
  int option;
  std::string filename, content, newName;
    
  do {
    showMenu();
    std::cin >> option;
    std::cin.ignore();
        
    switch (option) {
      case 1: // Crear archivo
        std::cout << "Nombre del archivo: ";
        std::getline(std::cin, filename);
        fs->create(filename);
        break;
                
      case 2: // Agregar contenido
        std::cout << "Nombre del archivo: ";
        std::getline(std::cin, filename);
        std::cout << "Contenido: ";
        std::getline(std::cin, content);
        fs->add(filename, content);
        break;
                
      case 3: // Mostrar contenido
        std::cout << "Nombre del archivo: ";
        std::getline(std::cin, filename);
        fs->printInodeContent(filename);
        break;
                
      case 4: // Listar archivos
        fs->fileList();
        break;
                
      case 5: // Eliminar archivo
        std::cout << "Nombre del archivo: ";
        std::getline(std::cin, filename);
        fs->deleteFile(filename);
        break;
      case 6: // Mostrar superbloque
        fs->printSB();
        break;

      case 7:
        std::cout << "Nombre del archivo: ";
        std::getline(std::cin, filename);
        std::cout << "Nuevo nombre del archivo: ";
        std::getline(std::cin, newName);
        fs->changeName(filename, newName);
        break;
                
      case 0: // Salir
        std::cout << "¡Hasta luego!\n";
        break;
                
      default:
        std::cout << "Invalid input\n";
    }
        
  } while (option != 0);
    
  delete fs;
  return 0;
}