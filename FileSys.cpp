// CPSC 3500: File System
// Implements the file system commands that are available to the shell.

#include <cstring>
#include <iostream>
using namespace std;

#include "FileSys.h"
#include "BasicFileSys.h"
#include "Blocks.h"

// mounts the file system
void FileSys::mount() {
  bfs.mount();
  curr_dir = 1;
}

// unmounts the file system
void FileSys::unmount() {
  bfs.unmount();
}

// make a directory
void FileSys::mkdir(const char *name)
{
  dirblock_t dirblock;
  bfs.read_block(curr_dir,(void *) &dirblock);
  
  short free = bfs.get_free_block();
  
  if(free != 0)
  {
    if(dirblock.num_entries <= MAX_DIR_ENTRIES)
    {
      if(strlen(name) <= MAX_FNAME_SIZE)
      {
        if(!Exists(name))
        { 
          dirblock.dir_entries[dirblock.num_entries].block_num = free;
          dirblock_t temp;
          bfs.read_block(free, (void *) &temp);
          
          for(int i = 0; i < strlen(name); i++)
          {
            dirblock.dir_entries[dirblock.num_entries].name[i] = name[i];
          }
          
          dirblock.num_entries++;
          temp.magic = DIR_MAGIC_NUM;
          bfs.write_block(free, (void *) &temp);
          bfs.write_block(curr_dir, (void *) &dirblock);
        }
        else
          cout << "\"" <<  name << "\"" << " already exists." << endl;
      }
      else
        cout << "Name too long." << endl;
    }
    else
      cout << "Directory is full." << endl;
  }
  else
    cout << "Disk is full." << endl;
}

// switch to a directory
void FileSys::cd(const char *name)
{
  dirblock_t dirblock;
  bfs.read_block(curr_dir,(void *) &dirblock);
  
  if(Exists(name))
  {
    if(IsDirectory(dirblock.dir_entries[ExistsWhere(name)].block_num))
      curr_dir = dirblock.dir_entries[ExistsWhere(name)].block_num;
    
    else
      cout << "\"" << name << "\"" << " is not a directory." << endl;
  }
  
  else
    cout << "No directory found with name \"" << name << "\"." << endl;
}

// switch to home directory
void FileSys::home() 
{
	curr_dir = 1;
}

// remove a directory
void FileSys::rmdir(const char *name)
{
  dirblock_t dirblock;
  bfs.read_block(curr_dir,(void *) &dirblock);
  
  if(Exists(name))
  {
    dirblock_t temp;
    bfs.read_block(dirblock.dir_entries[ExistsWhere(name)].block_num,(void *) &temp);
    
    if(temp.magic == DIR_MAGIC_NUM)
    {
      if(temp.num_entries == 0)
      {
        int index = ExistsWhere(name);
        for(int i = 0; i < MAX_FNAME_SIZE; i++)
        {
          dirblock.dir_entries[index].name[i] = '\0';
        }
        
        dirblock.num_entries--;
        for(int i = index; i < dirblock.num_entries; i++)
        {
          for(int k = 0; k < MAX_FNAME_SIZE; k++)
            dirblock.dir_entries[i].name[k] = dirblock.dir_entries[i + 1].name[k];
          dirblock.dir_entries[i].block_num = dirblock.dir_entries[i + 1].block_num;
        }
        
        for(int i = 0; i < MAX_FNAME_SIZE; i++)
        {
          dirblock.dir_entries[dirblock.num_entries + 1].name[i] = '\0';
        }
        dirblock.dir_entries[dirblock.num_entries + 1].block_num = 0;
        
        bfs.reclaim_block(dirblock.dir_entries[index].block_num);
        
        bfs.write_block(curr_dir, (void *) &dirblock);
      }
      else
        cout << "\"" << name << "\"" << " is not empty." << endl;
    }
    
    else
      cout << "\"" << name << "\"" << " is not a direcotry." << endl;
  }
  
  else
    cout << "\"" << name << "\"" << " does not exist." << endl;
}

// list the contents of current directory
void FileSys::ls()
{
  dirblock_t dirblock;
  bfs.read_block(curr_dir,(void *) &dirblock);
  
  if(dirblock.num_entries > 0)
  {
    for(int i = 0; i < dirblock.num_entries; i++)
    {
      cout << dirblock.dir_entries[i].name << " ";
    }
    cout << endl;
  }
}

// create an empty data file
void FileSys::create(const char *name)
{
  dirblock_t dirblock;
  bfs.read_block(curr_dir,(void *) &dirblock);
  
  short free = bfs.get_free_block();
  
  if(free != 0)
  {
    if(dirblock.num_entries <= MAX_DIR_ENTRIES)
    {
      if(strlen(name) <= MAX_FNAME_SIZE)
      {
        if(!Exists(name))
        {    
          dirblock.dir_entries[dirblock.num_entries].block_num = free;
          
          inode_t inode;
          
          for(int i = 0; i < strlen(name); i++)
          {
            dirblock.dir_entries[dirblock.num_entries].name[i] = name[i];
          }
          
          dirblock.num_entries++;
          
          inode.magic = INODE_MAGIC_NUM;
          inode.size = 0;
          
          for(int i = 0; i < MAX_DATA_BLOCKS; i++)
          {
            inode.blocks[i] = 0;
          }
          
          short FreeData = bfs.get_free_block();
          inode.blocks[0] = FreeData;
          
          bfs.write_block(curr_dir, (void *) &dirblock);
          bfs.write_block(free, (void *) &inode);
        }
        
        else
          cout << "File already exists." << endl;
      }
      
      else
        cout << "File name too long." << endl;
    }
    else
      cout << "Directory is full." << endl;
  }
  else
    cout << "Disk is full." << endl;  
}

// append data to a data file
void FileSys::append(const char *name, const char *data)
{
  dirblock_t dirblock;
  bfs.read_block(curr_dir,(void *) &dirblock);
  
  inode_t inode;
  bfs.read_block(dirblock.dir_entries[ExistsWhere(name)].block_num, (void *) &inode);
  
  if((strlen(data) + inode.size) <= MAX_FILE_SIZE)
  {
    if(Exists(name))
    {
      if(IsINODE(dirblock.dir_entries[ExistsWhere(name)].block_num))
      {
        datablock_t datablock;
        
        short lastBlock = 0;
        short numNewBlocks = 0;
        short lastIndex = 0;
        
        
        for(int i = 1; i < MAX_DATA_BLOCKS; i++)
        {
          if(inode.blocks[i] != 0)
            lastBlock++;
          else
            break;
        }
        
        bfs.read_block(inode.blocks[lastBlock], (void *) &datablock);
        for(lastIndex = 0; lastIndex < BLOCK_SIZE; lastIndex++)
        {
          if(datablock.data[lastIndex] == '\0')
            break;
        }
        for(int i = 0; i < strlen(data); i++)
        {
          if(DataBlockFull(inode.blocks[lastBlock]))
          {
            numNewBlocks++;
            bfs.write_block(inode.blocks[lastBlock], (void *) &datablock);
            lastBlock++;
            short free = bfs.get_free_block();
            if(free != 0)
              inode.blocks[lastBlock] = free;
            else
            {
              cout << "Disk is full." << endl;
              break;
            }
            bfs.write_block(dirblock.dir_entries[ExistsWhere(name)].block_num, (void *) &inode);
            bfs.read_block(inode.blocks[lastBlock], (void *) &datablock);
            lastIndex = 0;
          }
          inode.size++;
          datablock.data[lastIndex] = data[i];
          lastIndex++;
          bfs.write_block(inode.blocks[lastBlock], (void *) &datablock);
        }
        bfs.write_block(dirblock.dir_entries[ExistsWhere(name)].block_num, (void *) &inode);
      }
      
      else
        cout  << "\"" << name << "\"" << " is not a file." << endl;
    }
    else
      cout << "\"" << name << "\"" << " does not exist." << endl;
  }
  else
    cout << "Append exceeds maximum file size." << endl;
}

// display the contents of a data file
void FileSys::cat(const char *name)
{
  dirblock_t dirblock;
  bfs.read_block(curr_dir,(void *) &dirblock);
  
  if(Exists(name))
  {
    if(IsINODE(dirblock.dir_entries[ExistsWhere(name)].block_num))
    {
      inode_t inode;
      bfs.read_block(dirblock.dir_entries[ExistsWhere(name)].block_num, (void *) &inode);
      
      if(inode.size > 0)
      {
        int blocks = (inode.size / BLOCK_SIZE) + 1;
        for(int i = 0; i < blocks; i++)
        {
          datablock_t datablock;
          bfs.read_block(inode.blocks[i], (void *) &datablock);
          for(int k = 0; k < BLOCK_SIZE; k++)
          {
            if(datablock.data[k] == '\0')
              break;
            
            cout << datablock.data[k];
          }
        }
        
        cout << endl;
      }
      
      else
        cout << "File empty." << endl;
    }
    
    else
      cout << "\"" << name << "\"" << " is not a file." << endl;
  }
  
  else
    cout << "\"" << name << "\"" << " does not exist." << endl;
}

// display the last N bytes of the file
void FileSys::tail(const char *name, unsigned int n)
{
  dirblock_t dirblock;
  bfs.read_block(curr_dir,(void *) &dirblock);
  
  if(Exists(name))
  {
    if(IsINODE(dirblock.dir_entries[ExistsWhere(name)].block_num))
    {
      inode_t inode;
      bfs.read_block(dirblock.dir_entries[ExistsWhere(name)].block_num, (void *) &inode);
      
      if(n < inode.size)
      {
        short numBlocks = 0;
        short startIndex = inode.size - n;
        if(startIndex >= BLOCK_SIZE)
        {
          numBlocks--;
          for(int i = 0; i < startIndex; i++)
          {
            if(i % BLOCK_SIZE == 0)
              numBlocks++;
          }
          startIndex -= numBlocks * BLOCK_SIZE;
        }
        
        for(int i = numBlocks; i < (inode.size / BLOCK_SIZE) + 1; i++)
        {
          datablock_t datablock;
          bfs.read_block(inode.blocks[i], (void *) &datablock);
          for(int k = startIndex; k < BLOCK_SIZE; k++)
          {
            if(datablock.data[k] == '\0')
              break;
            
            cout << datablock.data[k];
          }
          
          startIndex = 0;
        }
        
        cout << endl;
      }
      
      else
        cat(name);
    }
  }
}

// delete a data file
void FileSys::rm(const char *name)
{
  dirblock_t dirblock;
  bfs.read_block(curr_dir,(void *) &dirblock);
  
  if(Exists(name))
  {
    if(IsINODE(dirblock.dir_entries[ExistsWhere(name)].block_num))
    {
      int index = ExistsWhere(name);
      
      inode_t inode;
      bfs.read_block(dirblock.dir_entries[index].block_num, (void *) &inode);
      
      for(int i = 0; i < (inode.size / BLOCK_SIZE) + 1; i++)
      {
        bfs.reclaim_block(inode.blocks[i]);
        inode.blocks[i] = 0;
      }
      
      dirblock.num_entries--;
      
      for(int i = index; i < dirblock.num_entries; i++)
      {
        for(int k = 0; k < MAX_FNAME_SIZE; k++)
          dirblock.dir_entries[i].name[k] = dirblock.dir_entries[i + 1].name[k];
        dirblock.dir_entries[i].block_num = dirblock.dir_entries[i + 1].block_num;
      }
      
      for(int i = 0; i < MAX_FNAME_SIZE; i++)
      {
        dirblock.dir_entries[dirblock.num_entries + 1].name[i] = '\0';
      }
      dirblock.dir_entries[dirblock.num_entries + 1].block_num = 0;
      
      bfs.reclaim_block(dirblock.dir_entries[index].block_num);
      bfs.write_block(curr_dir,(void *) &dirblock);
    }
    
    else
      cout << "\"" << name << "\"" << " is not a file." << endl;
  }
  
  else
    cout << "\"" << name << "\"" << " does not exist." << endl;
}

// display stats about file or directory
void FileSys::stat(const char *name)
{
  dirblock_t dirblock;
  bfs.read_block(curr_dir,(void *) &dirblock);
  
  if(Exists(name))
  {
    if(IsDirectory(dirblock.dir_entries[ExistsWhere(name)].block_num))
    {
      cout << "Directory Name: " << dirblock.dir_entries[ExistsWhere(name)].name << "/" << endl;
      cout << "Directory Block: " << dirblock.dir_entries[ExistsWhere(name)].block_num << endl;
    }
    
    if(IsINODE(dirblock.dir_entries[ExistsWhere(name)].block_num))
    {
      inode_t file;
      bfs.read_block(dirblock.dir_entries[ExistsWhere(name)].block_num, (void *) &file);
      
      cout << "File Name: " << dirblock.dir_entries[ExistsWhere(name)].name << endl;
      cout << "I-Node Block: " << dirblock.dir_entries[ExistsWhere(name)].block_num << endl;
      cout << "Bytes in File: " << file.size << endl;
      cout << "Number of Blocks: " << (file.size/BLOCK_SIZE) + 2 << endl;
      cout << "First Block: " << file.blocks[0] << endl;
    }
  }
}

// HELPER FUNCTIONS (optional)
bool FileSys::IsDirectory(short block)
{
  dirblock_t dirblock;
  bfs.read_block(block,(void *) &dirblock);
  
  if(dirblock.magic == DIR_MAGIC_NUM)
    return true;
  
  else
    return false;
}

bool FileSys::IsINODE(short block)
{
  inode_t inode;
  bfs.read_block(block,(void *) &inode);
  
  if(inode.magic == INODE_MAGIC_NUM)
    return true;
  
  else
    return false;
}

bool FileSys::Exists(const char *name)
{
  dirblock_t dirblock;
  bfs.read_block(curr_dir,(void *) &dirblock);
  
  bool exists = false;
  
  for(int i = 0; i < dirblock.num_entries; i++)
  {
    if(strlen(name) == strlen(dirblock.dir_entries[i].name))
    {
      for(int k = 0; k < strlen(name); k++)
      {
        if(name[k] != dirblock.dir_entries[i].name[k])
        {
          exists = false;
          break;
        }
        else
          exists = true;
      }
    }
    
    else
      exists = false;
    
    if(exists)
      break;
  }
  
  return exists;
}

short FileSys::ExistsWhere(const char *name)
{
  dirblock_t dirblock;
  bfs.read_block(curr_dir,(void *) &dirblock);
  
  bool exists = false;
  int i;
  
  for(i = 0; i < dirblock.num_entries; i++)
  {
    if(strlen(name) == strlen(dirblock.dir_entries[i].name))
    {
      for(int k = 0; k < strlen(name); k++)
      {
        if(name[k] != dirblock.dir_entries[i].name[k])
        {
          exists = false;
          break;
        }
        else
          exists = true;
      }
    }
    
    else
      exists = false;
    
    if(exists)
      break;
  }
  
  return i;
}

bool FileSys::DataBlockFull(short block)
{
  datablock_t datablock;
  bfs.read_block(block, (void *) &datablock);

  for(int i = 0; i < BLOCK_SIZE; i++)
  {
    if(datablock.data[i] == '\0')
      return false;
  }
  
  return true;
}