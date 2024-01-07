/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"
#include "synchconsole.h"
#include "serversocket.h"
#include "openfile.h"
#include "fdt.h"
#include "ptable.h"
#include "stable.h"

/**
 * Systemcall interface
 * These are the operations the Nachos kernel needs to support
 */
void SysHalt()
{
  kernel->interrupt->Halt();
}

/**
 * Print a string on the console
 * @param string The null-terminated string to print
 * @see ConsoleDriver::PutString
 */
int SysPrintString(char *string)
{
  int size = strnlen(string, MAX_ALLOWED_BUFFER_SIZE);

  kernel->synchConsoleOut->PutString(string, size);
  // kernel->GetConsole()->PutString(string);
  return size;
}

void exitWithError(char* msg){
  SysPrintString(msg);
  kernel->interrupt->Halt();
}

int SysCreate(char *filename)
{
  int res;

  if (kernel->fileSystem->Create(filename))
  {
    res = 0;
    DEBUG(dbgFile, "\n\tCreated file " << filename);
  }
  else
  {
    res = -1;
    DEBUG(dbgFile, "\n\tCannot create file " << filename);
  }

  return res;
}

int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

/**
 * Print a character on the console. If the console is not initialized yet,
 * do nothing.
 * @param c The character to print
 * @see ConsoleDriver::PutChar
 */
void SysPrintChar(char c)
{
  kernel->synchConsoleOut->PutChar(c);
  // kernel->GetConsole()->PutChar(c);
}

/**
 * Open a file with a specified OpenMode
 * @param filename The name of the file to open
 * @param mode (RO = 0, RW = 1)
 * RO: = 0 read only
 * RW: = 1 read and write
 * @return The file descriptor of the opened file, -1 if an error occured
 */
OpenFileID SysOpen(FileName filename, OpenMode mode)
{
  //check if file is opening or not
  if(FileDescriptorTable::getInstance()->isOpen(filename)){
    DEBUG(dbgFile, "\n\tCannot open file " << filename << " because it is opening in fdt");
    return -1;
  }

  if(!FileDescriptorTable::getInstance()->hasFreeFileDescriptor())
  {
    return -1;
  }

  DyOpenFile *file = new DyOpenFile(filename, mode);
  int res = file->open();
  if(res == -1){
    DEBUG(dbgFile, "\n\tCannot create file " << filename);
    delete file;
    return -1;
  }else{
    int fd = FileDescriptorTable::getInstance()->Add(file);
    DEBUG(dbgFile, "\n\tCreated file " << filename << " with fd " << fd);
    return fd;
  }
}

/*
 * Close current open file in the file descriptor table
 * @param id The file descriptor of the file to close
 * @return 0 on success, -1 if an error occured
 */
int SysClose(OpenFileID id)
{
  int res = FileDescriptorTable::getInstance()->close(id);

  if(res == -1){
    DEBUG(dbgFile, "\n\tCannot close file " << id);
  }else{
    DEBUG(dbgFile, "\n\tClosed file " << id);
  }

  return res;
}

/**
 * Read a string from the console
 * @param buffer The buffer to read into
 * @param size The size of the buffer
 * @return The number of characters read, -1 if an error occured
*/
int consoleRead(char *buffer, int size)
{
  DEBUG(dbgFile, "\n\tReading from console with size " << size);
  int i = 0;
  char c;

  while (i < size)
  {
    
    c = kernel->synchConsoleIn->GetChar();
    DEBUG(dbgFile, "\n\tRead character " << c << " from console "<< i << " times");

    if (c == EOF || c == '\0' || c == '\n')
    {
      DEBUG(dbgFile, "\n\tEnd string reached");
      buffer[i] = '\0';
      break;
    }

    buffer[i] = c;
    ++i;
  }

  DEBUG(dbgFile, "\n\tValue: " << buffer);

  if(i > MAX_ALLOWED_BUFFER_SIZE){
    exitWithError("String is too large");
  }

  return i;
}

/**
 * Read a string from a file
 * @param buffer The buffer to read into
 * @param size The size of the buffer
 * @param fd The file descriptor of the file to read from
 * @return The number of characters read, -1 if an error occured
*/
int SysRead(char *buffer, int size, OpenFileID fd)
{
  //check if size > MAX_ALlOWED_SIZE
  if(size > MAX_ALLOWED_BUFFER_SIZE){
    exitWithError("Buffer size is too large");
  }
  // check if fd is STDIN
  if (fd == STDIN)
  {
    return consoleRead(buffer, size);
  }

  int res;
  OpenStream *file = FileDescriptorTable::getInstance()->getStream(fd);

  // check for validity of file
  if (file == NULL)
  {
    DEBUG(dbgFile, "\n\tCannot read from file because it's not opened" << fd);
    res = -1;
  }
  else
  {
    res = file->read(buffer, size);
    DEBUG(dbgFile, "\n\tRead " << res << " characters from fd " << fd);
    DEBUG(dbgFile, "\n\tValue: " << buffer);  
  }

  if(res > MAX_ALLOWED_BUFFER_SIZE){
    exitWithError("String is too large");
  }

  return res;
}

int consoleWrite(char* buffer, int size){
  DEBUG(dbgFile, "\n\tWriting to console with size " << size);
  int i = 0;
  char c;

  while (i < size)
  {
    c = buffer[i];
    //DEBUG(dbgFile, "\n\tWriting character " << c << " to console "<< i << " times");

    if (c == '\0')
    {
      DEBUG(dbgFile, "\n\tEnd of file reached");
      break;
    }

    kernel->synchConsoleOut->PutChar(c);
    ++i;
  }

  DEBUG(dbgFile, "\n\tValue: " << buffer);
  return i;
}

/**
 * Write a string to a file
 * @param buffer The buffer to write from
 * @param size The size of the buffer
 * @param fd The file descriptor of the file to write to
 * @return The number of characters written, -1 if an error occured
*/
int SysWrite(char* buffer, int size, OpenFileID fd){
  DEBUG(dbgFile, "\n\tSysWrite received buffer: " << buffer << " with size " << size);
  //check if size > MAX_ALlOWED_SIZE
  if(size > MAX_ALLOWED_BUFFER_SIZE){
    exitWithError("Buffer size is too large");
  }
  // check if fd is STDOUT
  if (fd == STDOUT)
  {

    return consoleWrite(buffer, size);
  }

  DEBUG(dbgFile, "\n\tWriting to fd " << fd << " with size " << size);

  int res;
  OpenStream *file = FileDescriptorTable::getInstance()->getStream(fd);

  // check for validity of file
  if (file == NULL)
  {
    res = -1;
    DEBUG(dbgFile, "\n\tCannot write to file because it's not opened" << fd);
  }else if(!file->canWrite()){
    res = -1;
    DEBUG(dbgFile, "\n\tCannot write to file because it's opened in readonly mode" << fd);
  }
  else
  {
      DEBUG(dbgFile, "case 3 ");
      buffer[size] = '\0';
    res = file->write(buffer, size);
    DEBUG(dbgFile, "\n\tWrote " << res << " characters to fd " << fd);
  }
  
  DEBUG(dbgFile, "\n\tWrote " << res << " characters to fd " << fd);
  DEBUG(dbgFile, "\n\tValue: " << buffer);
  return res;
}


/**
 * Seek to a specified position in a file
 * @param pos The position to seek to
 * @param fd The file descriptor of the file to seek in
 * @return The new position in the file, -1 if an error occured
*/
int SysSeek(int pos, OpenFileID fd){
  int res = pos;
  OpenStream *file = FileDescriptorTable::getInstance()->getStream(fd);

  // check for validity of file
  if(fd == STDIN || fd == STDOUT){
    DEBUG(dbgFile, "\n\tCannot seek on STDIN or STDOUT");
    exitWithError("Cannot seek on STDIN or STDOUT");
  }
  else if (file == NULL)
  {
    DEBUG(dbgFile, "\n\tCannot seek on file because it's not opened" << fd);
    return -1;
  }
  else if(res == -1){//seek to EOF
    pos = file->length();
  }

  file->seek(pos);
  res = pos;

  DEBUG(dbgFile, "\n\tSeeked to position " << pos << " in fd " << fd);
  return res;
}

/**
 * Remove a file give a given name
 * @param filename The name of the file to remove
 * @return 0 on success, -1 if an error occured
 */
int SysRemove(char* filename){
  int res;

  //check if file is opening or not
  if(FileDescriptorTable::getInstance()->isOpen(filename)){
    DEBUG(dbgFile, "\n\tCannot remove file " << filename << " because it is opening in fdt");
    return -1;
  }

  if (kernel->fileSystem->Remove(filename))
  {
    res = 0;
    DEBUG(dbgFile, "\n\tRemoved file " << filename);
  }
  else
  {
    res = -1;
    DEBUG(dbgFile, "\n\tCannot remove file (linux)" << filename);
  }

  return res;
}

/**
 * Create a new TCP socket
 * @return The file descriptor of the socket, -1 if an error occured
 * 
*/
int SysSocketTCP(){
  int fd = FileDescriptorTable::getInstance()->Add(new Socket());
  return fd;
}

/**
 * Connect to a TCP server
 * @param fd The file descriptor of the socket
 * @param ip The ip address of the server
 * @param port The port of the server
 * @return 0 on success, -1 if an error occured
*/
int SysConnect(int fd, char* ip, int port){
  Socket* socket = (Socket*) FileDescriptorTable::getInstance()->getStream(fd);

  if(socket == NULL){
    DEBUG(dbgFile, "\n\tCannot connect to server because socket is not opened");
    return -1;
  }

  int res = socket->connect(ip, port);
  return res;
}

/**
 * Send data to a TCP server
 * @param fd The file descriptor of the socket
 * @param buffer The buffer to send
 * @param len The length of the buffer
 * @return The number of bytes sent, -1 if an error occured
*/
int SysSend(int fd, char* buffer, int len){
  //check if size > MAX_ALlOWED_SIZE
  if(len > MAX_ALLOWED_BUFFER_SIZE){
    exitWithError("Buffer size is too large");
  }
  
  Socket* socket = (Socket*) FileDescriptorTable::getInstance()->getStream(fd);

  if(socket == NULL){
    DEBUG(dbgFile, "\n\tCannot send data to server because socket is not opened");
    return -1;
  }

  int res = socket->write(buffer, len);
  return res;
}

int SysReceive(int socketID, char* buffer, int size){
  //check if size > MAX_ALlOWED_SIZE
  if(size > MAX_ALLOWED_BUFFER_SIZE){
    exitWithError("Buffer size is too large");
  }
  Socket* socket = (Socket*) FileDescriptorTable::getInstance()->getStream(socketID);

  if(socket == NULL){
    DEBUG(dbgFile, "\n\tCannot receive data from server because socket is not opened");
    return -1;
  }

  int res = socket->read(buffer, size);
  return res;
}

/**
 * Stop SocketServer
*/
int SysDisconnect(int socketID){
  exitWithError("Disconnect is not implemented");
  return 0;
}

/**
 * Create a new TCP server
 * @param port The port of the server
 * @return The file descriptor of the server, -1 if an error occured
*/
int SysServerCreate(int port){
  int fd = FileDescriptorTable::getInstance()->Add(new ServerSocket(port));
  return fd;
}

/**
 * Listen to a TCP server
 * @param ssID The file descriptor of the server
 * @return 0 on success, -1 if an error occured
 * @note This function is blocking
*/
int SysServerListen(int ssID){
  ServerSocket* serverSocket = (ServerSocket*) FileDescriptorTable::getInstance()->getStream(ssID);
  if(serverSocket == NULL){
    DEBUG(dbgFile, "\n\tCannot listen to server because socket is not opened");
    return -1;
  }

  int res = serverSocket->listen();
  return res;
}

/**
 * Add callback function to a TCP server
 * @param ss_fd The file descriptor of the server
*/
int SysSSPollAccept(int ss_fd){
  exitWithError("SSPollAccept is not implemented");
  ServerSocket* serverSocket = (ServerSocket*) FileDescriptorTable::getInstance()->getStream(ss_fd);
  if(serverSocket == NULL){
    DEBUG(dbgFile, "\n\tCannot listen to server because socket is not opened");
    return -1;
  }

  //int res = serverSocket->pollAccept();
  return 0;
}

int SysExec(char* filename){
  DEBUG(dbgThread, "\n\tSysExec received filename: " << filename);
  return kernel->pTab->ExecUpdate(filename);
}

void SysExit(int status){
  DEBUG(dbgThread, "\n\tSysExit received status: " << status);
  kernel->pTab->ExitUpdate(status);
}

int SysJoin(int pid){
  DEBUG(dbgThread, "\n\tSysJoin received pid: " << pid);
  return kernel->pTab->JoinUpdate(pid);
}

int SysCreateSem(char* name, int permits){
  DEBUG(dbgThread, "\n\tSysCreateSem received name: " << name << " with permits " << permits);
  return kernel->semTab->Create(name, permits);
}

int SysWait(char* name){
  DEBUG(dbgThread, "\n\tSysWait received name: " << name);
  return kernel->semTab->Wait(name);
}

int SysSignal(char* name){
  DEBUG(dbgThread, "\n\tSysSignal received name: " << name);
  return kernel->semTab->Signal(name);
}

int SysExecV(int argc, char* argv[]){
  ASSERT(argc>0);
  DEBUG(dbgThread, "\n\tSysExecV received filename: " << argv[0] << " with argc " << argc);
  return kernel->pTab->ExecVUpdate(argc, argv);
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
