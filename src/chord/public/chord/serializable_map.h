#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include<fstream>

using namespace std;
namespace Chord
{

template<class Key, class Value>
class serializable_map : public std::map<Key, Value> {
private:
  size_t offset;

  template<class T>
  void write(std::stringstream &ss, T &t) {
    ss.write((char*)(&t), sizeof(t));
  }

  void write(std::stringstream &ss, std::string &str) {
    size_t size = str.size();
    ss.write((char*)(&size), sizeof(size));
    ss.write((char*)(str.data()), str.length());
  }

  template<class T>
  void read(std::vector<char> &buffer, T &t) {
    t = (T)(*(buffer.data() + offset));
    offset += sizeof(T);
  }

  void read(std::vector<char> &buffer, std::string &str) {
    size_t size = (int)(*(buffer.data() + offset));
    offset += sizeof(size_t);
    std::string str2(buffer.data() + offset, buffer.data() + offset + size);
    str = str2;
    offset += size;
  }
public:
  std::vector<char> serialize() {
    std::vector<char> buffer;
    std::stringstream ss;
    for (auto &i : (*this)) {
      Key str = i.first;
      Value value = i.second;
      write(ss, str);
      write(ss, value);
    }
    size_t size = ss.str().size();
    buffer.resize(size);
    ss.read(buffer.data(), size);
    return buffer;
  }
  void deserialize(std::vector<char> &buffer) {
    offset = 0;
    while (offset < buffer.size()) {
      Key key;
      Value value;
      read(buffer, key);
      read(buffer, value);
      (*this)[key] = value;
    }
  }
  void show(void) {
    for (auto &i : (*this)) {
      std::cout << i.first << ":" << i.second << std::endl;
    }
    std::cout << std::endl;
  }
};

/*
void
test1(void) {
  std::cout << "string->int" << std::endl;
  serializable_map<std::string, int> m;
  m["test1"] = 1;
  m["test2"] = 2;
  m["test3"] = 3;
  m.show();
  std::vector<char> buffer = m.serialize();
  ofstream os ("files.dat", ios::binary);
  int size1 = buffer.size();
  cout << "Size of the vector buffer: " << size1 << endl;
  os.write((const char*)&size1, 4);
  os.write((const char*)&buffer[0], size1 * sizeof(char));
  os.close();

  std::vector<char> buffer1;
  serializable_map<std::string, int> m2;
  ifstream is("files.dat", ios::binary);
  int size2;
  is.read((char*)&size2, 4);
  buffer1.resize(size2);
  is.read((char*)&buffer1[0], size2 * sizeof(char));
  m2.deserialize(buffer1);
  m2.show();
}

void
test2(void) {
  std::cout << "int->string" << std::endl;
  serializable_map<int, std::string> m;
  m[1] = "test1";
  m[2] = "test2";
  m[3] = "test3";
  std::vector<char> buffer = m.serialize();
  serializable_map<int, std::string> m2;
  m2.deserialize(buffer);
  m2.show();
}

void
test3(void) {
  std::cout << "int->int" << std::endl;
  serializable_map<int, int> m;
  m[1] = 1;
  m[2] = 2;
  m[3] = 3;
  std::vector<char> buffer = m.serialize();
  serializable_map<int, int> m2;
  m2.deserialize(buffer);
  m2.show();
}

int
main(void) {
  test1();
  //test2();
  //test3();
}*/
} // namespace Chord
