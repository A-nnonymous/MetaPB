#include <bits/stdc++.h>
#include <xgboost/c_api.h>
using namespace std;

/*
class Base {
public:
  virtual ~Base() {}

  virtual std::unique_ptr<Base> clone() const = 0;
  virtual void display() const = 0;

  virtual std::vector<std::unique_ptr<Base>> split(int n) const {
    std::vector<std::unique_ptr<Base>> result;
    for (int i = 0; i < n; ++i) {
      result.push_back(clone());
    }
    return result;
  }
  void printType() { cout << typeid(decltype(this)).name() << endl; }
};

class Derived : public Base {
public:
  std::unique_ptr<Base> clone() const override {
    return std::make_unique<Derived>(*this);
  }

  void display() const override {
    std::cout << "Derived class1 display function" << std::endl;
  }

};
*/
class Base {
public:
  typedef struct {
    BoosterHandle timeConsumeModelHandle;
    BoosterHandle energyConsumeModelHandle;
  } PerfModel;
  inline static PerfModel CPUPerfModel;
  inline static PerfModel DPUPerfModel;

  struct ArgMap {
    map<string, float> attr;
  };

  struct PerfArgs : ArgMap {
    PerfArgs() : ArgMap() {
      // TODO: structural binding + iterator refractor
      attr["a"] = 1;
    }
  };
  inline static PerfArgs perfArgs;

  vector<float> getPerfSignature() {
    vector<float> result;
    for (const auto &[k, v] : perfArgs.attr) {
      result.emplace_back(v);
    }
    return result;
  }

  static void setCPUPerfModel(const BoosterHandle tModel,
                              const BoosterHandle eModel) {
    CPUPerfModel.timeConsumeModelHandle = tModel;
    CPUPerfModel.energyConsumeModelHandle = eModel;
  }
  static void setDPUPerfModel(const BoosterHandle tModel,
                              const BoosterHandle eModel) {
    DPUPerfModel.timeConsumeModelHandle = tModel;
    DPUPerfModel.energyConsumeModelHandle = eModel;
  }
};

class Derived : public Base {
public:
  struct PerfArgs : Base::PerfArgs {
    PerfArgs() : Base::PerfArgs() { attr["b"] = 1; }
  };
  inline static PerfArgs pa;
};

int main() {
  /*
  Derived obj;
  int n = 5;
  std::vector<std::unique_ptr<Base>> result = obj.split(n);
  for (const auto &ptr : result) {
    ptr->display();
  }
*/
  /*
  Derived d;
  BoosterHandle bh;
  d.setCPUPerfModel(bh, bh);
  for (auto &[k, v] : d.pa.attr) {
    cout << k << endl;
  }
  */
  enum class te { one, two, three };

  std::vector<te> a(3, te::one);
  for (const auto &ai : a) {
    if (ai == te::one) {
      cout << 1 << endl;
    }
    if (ai == te::two) {
      cout << 2 << endl;
    }
    if (ai == te::three) {
      cout << 3 << endl;
    }
  }
  return 0;
}
