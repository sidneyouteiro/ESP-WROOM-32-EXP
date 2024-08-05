#include <stdio.h>
#include <inttypes.h>
#include <iostream>
#include <vector>
#include <array>
#include <random>
#include <algorithm>
#include <chrono>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"


using namespace std;

typedef struct {
  long int num_bits;
  long int bitarray_size;
  uint64_t * bitarray;
} bitarray_t;

class Discriminator {
  public:
    Discriminator(int entrySize, int tupleSize) :
      entrySize(entrySize), tupleSize(tupleSize)
    {
      numRams = entrySize/tupleSize + ((entrySize%tupleSize) > 0);
      tuplesMapping = (int *)calloc(entrySize, sizeof(int));
      

      int i;
      for (i = 0; i < entrySize; i++) {
        tuplesMapping[i] = i;
      }
      

      
      //static std::default_random_engine engine(std::random_device{}());
      unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
      std::shuffle(tuplesMapping, tuplesMapping + entrySize, std::default_random_engine(seed));
      rams = (bitarray_t *) calloc(numRams, sizeof(bitarray_t));

      int num_bits = (1UL << tupleSize);
      cout << "TESTE num_bits " << num_bits << endl;
      long int bitarray_size = (num_bits >> 6);
      cout << "TESTE BEFORE bitarray_size " << bitarray_size << endl;
      bitarray_size += ((bitarray_size & 0x3F) > 0);
      cout << "TESTE AFTER bitarray_size " << bitarray_size << endl;

      cout << "TESTE TESTE TESTE Minimum free heap size: " << esp_get_minimum_free_heap_size() << " BYTES" << endl;
      cout << "TESTE TESTE TESTE free heap size: " << esp_get_free_heap_size() << " BYTES" << endl;
      for (i = 0; i < numRams; i++) {
        rams[i].num_bits = num_bits;
        rams[i].bitarray_size = bitarray_size;
        rams[i].bitarray = (uint64_t *) calloc(rams[i].bitarray_size, sizeof(uint64_t));
        if (rams[i].bitarray == NULL) {
          cout << "Não foi possivel alocar memoria por bitarray"<< endl;
          //exit(1);
        }
        cout << "TESTE BITARRAY " << rams[i].bitarray << endl;
      }
    }

    ~Discriminator()
    {
      free(rams);
      free(tuplesMapping);
    }

    void train(bitarray_t * data)
    {
      int i, j, k = 0, i1, i2;
      int addr_pos;
      uint64_t addr;

      for (i = 0; i < numRams; i++) {
        addr_pos = tupleSize-1;
        addr = 0;

        for (j = 0; j < tupleSize; j++) {
          i1 = tuplesMapping[k] >> 6;
          i2 = tuplesMapping[k] & 0x3F;

          addr |= (((data->bitarray[i1] & (1UL << i2)) >> i2) << addr_pos);
          addr_pos--;
          k++;
        }

        i1 = addr >> 6;
        i2 = addr & 0x3F;
        
        cout << "TESTE i " << i << endl;
        cout << "TESTE i1 " << i1 << endl;
        cout << "TESTE rams[i].bitarray[i1] " << rams[i].bitarray[i1] << endl;
        cout << "TESTE (1UL << i2) " << (1UL << i2) << endl;
        cout << "TESTE rams[i].bitarray[i1] | (1UL << i2) " << (rams[i].bitarray[i1] | (1UL << i2)) << endl;  

        rams[i].bitarray[i1] |= (1UL << i2);
      }
    }

    void train(const vector<bool>& data)
    {
      int i, j, k = 0, i1, i2;
      int addr_pos;
      uint64_t addr;
      
      for(i = 0; i < numRams; i++) {
        addr_pos = tupleSize-1;
        addr = 0;

        for (j = 0; j < tupleSize; j++) {
          addr |= (data[tuplesMapping[k]] << addr_pos);
          addr_pos--;
          k++;
        }

        i1 = addr >> 6;
        i2 = addr & 0x3F;
        cout << "TESTE i " << i << endl;
        cout << "TESTE i1 " << i1 << endl;
        cout << "TESTE rams[i].bitarray_size " << rams[i].bitarray_size << endl;
        cout << "TESTE (1UL << i2) " << (1UL << i2) << endl;
        cout << "TESTE rams[i].bitarray[i1] | (1UL << i2) " << (rams[i].bitarray[i1] | (1UL << i2)) << endl;

        rams[i].bitarray[i1] |= (1UL << i2);
      }
    }

    int rank(bitarray_t * data)
    {
      int rank=0;
      int i, j, k = 0, i1, i2;
      int addr_pos;
      uint64_t addr;

      for (i=0; i < numRams; i++) {
        addr_pos = tupleSize-1;
        addr = 0;

        for (j=0; j < tupleSize; j++) {
          i1 = tuplesMapping[k] >> 6;
          i2 = tuplesMapping[k] & 0x3F;
          addr |= (((data->bitarray[i1] & (1UL << i2)) >> i2) << addr_pos);
          addr_pos--;
          k++;
        }

        i1 = addr >> 6;
        i2 = addr & 0x3F;
        rank += (rams[i].bitarray[i1] & (1UL << i2)) >> i2;
      }

      return rank;
    }

    int rank(const vector<bool>& data)
    {
      int rank = 0;
      int i, j, k = 0, i1, i2;
      int addr_pos;
      uint64_t addr;

      for (i = 0; i < numRams; i++) {
        addr_pos = tupleSize-1;
        addr = 0;

        for (j = 0; (j < tupleSize) && (k < entrySize); j++) {
          addr |= (data[tuplesMapping[k]] << addr_pos);
          addr_pos--;
          k++;
        }

        i1 = addr >> 6;
        i2 = addr & 0x3F;
        rank += (rams[i].bitarray[i1] & (1UL << i2)) >> i2;
      }

      return rank;
    }

    void reset()
    {
      int i,j;
      
      for (i = 0; i < entrySize; i++) {
        tuplesMapping[i] = i;
      }

      std::shuffle(tuplesMapping, tuplesMapping + entrySize, std::default_random_engine(std::random_device{}()));

      for (i = 0; i < numRams; i++) {
        for (j = 0; j < rams[i].bitarray_size; j++) {
          rams[i].bitarray[j] = 0;
        }
      }
    }

  private:
    int entrySize;
    int tupleSize;
    int numRams;
    int * tuplesMapping;
    bitarray_t * rams;
};


class Wisard {
  public:
  Wisard(int entrySize, int tupleSize, int numDiscriminator):
  entrySize(entrySize), tupleSize(tupleSize), numDiscriminator(numDiscriminator)
  {
    int i;

    discriminators = vector<Discriminator*>(numDiscriminator);
    
    for (i=0; i < numDiscriminator; i++) {
      discriminators[i] = new Discriminator(entrySize, tupleSize);
    }
  }

  ~Wisard(){
    unsigned int i;
    for (i = 0; i < discriminators.size(); i++) {
      delete discriminators[i];
    }
  }

  void addDiscriminator()
  {
    discriminators.emplace_back(new Discriminator(entrySize, tupleSize));
    numDiscriminator++;
  }

  void train(const vector<vector<bool>>& data, const vector<int>& label)
  {
    unsigned int i;

    for (i = 0; i < label.size(); i++) {
      discriminators[label[i]]->train(data[i]);
    }
  }

  int rank(const vector<bool>& data)
  {
    int i;
    int label = 0;
    int max_resp = 0;
    int resp;

    for (i = 0; i < numDiscriminator; i++) {
      resp = discriminators[i]->rank(data);
      if (resp > max_resp) {
        max_resp = resp;
        label = i;
      }
    }

    return label;
  }

  std::vector<size_t> rank(const vector<vector<bool>>& data)
  {
    std::vector<size_t> a({data.size()});
    
    unsigned int i;
    int j, max_resp, resp;

    for(i = 0; i<data.size(); i++) {
      max_resp = 0;
      for(j = 0; j<numDiscriminator; j++) {
        resp = discriminators[j]->rank(data[i]);
        if(resp > max_resp) {
          max_resp = resp;
          a[i] = j;
        }
      }
    }
    return a;
  }

  void reset()
  {
    int i;
    for (i = 0; i < numDiscriminator; i++) {
      discriminators[i]->reset();
    }
  }

private:
  int entrySize;
  int tupleSize;
  int numDiscriminator;
  vector<Discriminator*> discriminators;
};

extern "C" void app_main(void)
{
  cout << "Hello world!" << endl;

  cout << "Minimum free heap size: " << esp_get_minimum_free_heap_size() << " BYTES" << endl;
  cout << "BEFORE ALL free heap size: " << esp_get_free_heap_size() << " BYTES" << endl;
  
  std::vector<bool> a = {1,1,1,0,0,0,0,0};
  std::vector<bool> b = {1,1,1,1,0,0,0,0};
  std::vector<bool> c = {0,0,0,0,1,1,1,1};
  std::vector<bool> d = {0,0,0,0,0,1,1,1};
  std::vector<int> y = {0,0,1,1};

  std::vector<vector<bool>> e = {};
  e.push_back(a);
  e.push_back(b);
  e.push_back(c);
  e.push_back(d);

  cout << "Minimum free heap size: " << esp_get_minimum_free_heap_size() << " BYTES" << endl;
  cout << "DATA free heap size: " << esp_get_free_heap_size() << " BYTES" << endl;

  Wisard * wisard = new Wisard(8, 3, 2);
  
  cout << "Minimum free heap size: " << esp_get_minimum_free_heap_size() << " BYTES" << endl;
  cout << "WISARD INSTANCE free heap size: " << esp_get_free_heap_size() << " BYTES" << endl;
  //wisard->train(e,y);
  cout << "Rank=" << endl;
  
  //std::vector<size_t> r = wisard->rank(e);
  //for (int i = 0; i < e.size(); i++){
  //  cout << r[i] << endl;
  //}

  cout << "AFTER free heap size: " << esp_get_free_heap_size() << " BYTES" << endl;
  cout << "Minimum free heap size: " << esp_get_minimum_free_heap_size() << " BYTES" << endl;

  for (int i = 10; i >= 0; i--) {
    cout << "Restarting in " << i << " seconds..." << endl;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  cout << "Restarting now." << endl;
  fflush(stdout);
  esp_restart();
}
