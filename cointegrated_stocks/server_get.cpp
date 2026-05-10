#include <cpr/cpr.h>
#include <string>
#include <iostream>
#include <server_get.h>

class server_get{

const std::string URL = "https://financialmodelingprep.com/api/v3/quote";
std::string stock_symbol;
std::string full_url = URL + stock_symbol;

public:
void cin_symbol(){
    std::cin>>stock_symbol;
}

std::string get_responce(const std::string &str){
    cpr::Response r = cpr::Get(cpr::Url{full_url});
    if(r.status_code != 200){
        std::cout<<"Get request error"<<r.status_code<<std::endl;
    }

    else return r.text;
}
};
