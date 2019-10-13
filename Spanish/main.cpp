#include <iostream>
#include "../vocab.cpp"
using namespace std;

int main(int argc, char *argv[]){
	string filename = "spanish_vocab";
	Session* session = new Session(filename);
	if (argc > 1 && string(argv[1]) == "stats"){
		session->stats_message();
		session->save();
		return 0;
	}
	session->round(DEFINITION);
	session->round(TARGET_LANG);
	return 0;
}