#include<iostream>

using namespace std; 

int main ()
{

    // cout<<"hello"<<endl; 
    uint32_t  a = 100; 

    void *p = &a; 
    // char *c = p;                      // C cho phép, C++ báo lỗi: phải ép kiểu rõ ràng
    char *c = static_cast<char*>(p);     // C++ bắt buộc ghi rõ ép kiểu
    cout<<*c<<endl;
    return 0; 
}