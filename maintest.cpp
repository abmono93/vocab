#include <iostream>
#include "vocab2.cpp"

int main(){
	VocabList A;
	A.loadSavedList("fakespanish");
	A.loadNewWords("fakespanish");
	A.printAll();
	A.saveToFile("fakespanish");
}
