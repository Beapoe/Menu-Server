#include <iostream>
#include <string>
#include <vector>

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

std::vector<char> stringToBinary(const std::string& str) {
    std::vector<char> bin;
    for (char ch : str) {
        std::string binStr = "";
        for (size_t i = 0; i < 8; ++i)
            binStr += (ch & (1 << i)) ? '1' : '0';
        bin.insert(bin.end(), binStr.rbegin(), binStr.rend());
    }
    return bin;
}

std::string encode(std::vector<char> bin){
	std::vector<std::vector<char>> groups;
	for(size_t i=0;i<bin.size();i+=6){
		std::vector<char> temp;
		for(size_t j=0;j<6&&(j+i)<bin.size();++j)
			temp.push_back(bin[i+j]);
		groups.push_back(temp);
	}

	if(!groups.empty() && groups.back().size()<6){
		for(int i=0;i<6-groups.back().size();++i){ 
		groups.back().push_back(0);
	  }
	}

	char pos = 0;
	std::string encoded = "";
	char bins = 0;
	for(auto g:groups){
		char power = 1;
		for(int i=g.size()-1;i>=0;i--){
			bins = g[i];
			std::cout<<"bins:"<<std::dec<<bins<<std::endl<<"power:"<<power;
			pos = bins*power;
			std::cout<<"pos:"<<std::dec<<pos;
			power = power*2;
		}
		encoded = encoded+base64_chars[pos];
		std::cout<<encoded<<std::endl;
		pos = 0;
	}
	return encoded;
}

int main(){
	std::string content = "休伯利安";
	std::cout<<content<<std::endl;
	std::string encoded = encode(stringToBinary(content));
	std::cout<<encoded<<std::endl;
	return 0;
}
