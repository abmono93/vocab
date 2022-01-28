#include <iostream>
#include "../vocab.cpp"
using namespace std;

int main(int argc, char *argv[]){
	Session session("spanish_vocab");
	if (argc > 1 && string(argv[1]) == "stats"){
		session.showStats();
		session.save();
		return 0;
	}
	session.round();
	session.round(true);
	return 0;
}
