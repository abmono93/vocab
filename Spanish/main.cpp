#include <iostream>
#include "../vocab.cpp"
using namespace std;

int main(int argc, char *argv[]){
	string filename = "spanish_vocab";
	Session session(filename);
	if (argc > 1 && string(argv[1]) == "stats"){
		session.showStats();
		return 0;
	}
	session.round();
	session.round(true);
	return 0;
}
