//задача 1 для лабораторной работы Ефимова ИКТС-2512
#include <iostream>
#include <cmath>
using namespace std;
int main() {
    setlocale(LC_CTYPE,"rus");
    double a, c, x;
    cout << "Введитe a, c, x: ";
    cin >> a >> c >> x;

    double chisl = sin(a*x)*sin(a*x) + 2*x;
    double zn = 1 - sqrt(3.14159);
    double y = cbrt(chisl / zn);

    cout << "y = " << y << endl;
    return 0;
}
