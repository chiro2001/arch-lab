//
// Created by chiro on 22-11-20.
//
#include <cstdio>
#include <vector>

using namespace std;

class Test {
public:
  int a;

  explicit Test(int a) : a(a) {}
};

int main() {
  vector<Test> v;
  for (int i = 0; i < 10; i++) v.emplace_back(Test(i));
  for (int i = 0; i < 10; i++) v[i].a++;
  for (int i = 0; i < 10; i++) printf("%d ", v[i].a);
  return 0;
}