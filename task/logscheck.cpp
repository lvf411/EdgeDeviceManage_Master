#include <iostream>
#include <fstream>
#include <string>
#include <strstream>
#include <unistd.h>

using namespace std;

int main(){
    string s;
    string target = "[done]";
    ifstream ifs;

    ifs.open("logs.txt", ios::binary);
    
    if(ifs.good())
    {
        cout << "read ok" << endl;
    }
    else 
    {
        cout << "read error" << endl;
        return 0;
    }
    while(1)
    {
        s.clear();
        while(getline(ifs, s))
        {
            //检查 [done]
            if(s.find(target, target.length()) != string::npos)
            {
                cout << "get [done]" << endl;
                cout << s << endl;
            }
            s.clear();
        }
        sleep(1);
        ifs.clear();
        ifs.seekg(0, ios::beg);
    }
    ifs.close();
    return 0;
}