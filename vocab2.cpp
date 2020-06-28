#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <fstream>

//Category indices
#define NEW 0
#define HARD 1
#define FAMILIAR 2
#define REVIEW 3
#define LEARNED 4

//Score boundaries
#define DEFAULT_SCORE 8
#define HARD_UPPERBOUND 8
#define LEARN_UPPERBOUND 11
#define MAX_SCORE 14

#define WORDS_PER_ROUND 10

using namespace std;

/*-------------------VocabWord------------------------------*/

struct VocabWord{
        VocabWord(string, int);
        string definition;
        int score;
        void changeScore(bool);
};

VocabWord::VocabWord(string definition, int score){
    this->definition = definition;
    this->score = score;
}

void VocabWord::changeScore(bool correct){
    if (correct){
        this->score = min(MAX_SCORE, score + 1);
    }else{
        if (score > LEARN_UPPERBOUND) score = LEARN_UPPERBOUND;
        this->score = max(0, score - 1);
    }
}

/*-------------------Category------------------------------*/

struct Category : public unordered_map<string, VocabWord*>{
    void print(int, ostream& dest = cout);
};

void Category::print(int categoryNum, ostream& dest){
    for (auto it=this->begin(); it != this->end(); it++){
        dest << it->first << "||";
        dest << it->second->definition << "||";
        dest << it->second->score << "|";
        dest << categoryNum;
        dest << endl;
    }
}

/*-------------------VocabList------------------------------*/

class VocabList : public vector<Category>{
    public:
        VocabList();
        void addToList(string, string, int score = -1, int category = -1);
        void loadSavedList(string);
        void loadNewWords(string);
        void saveToFile(string);
        void printAll();
        string lookup(string);
    private:
        int categorize(int);
        bool contains(string&);
        bool is_correct(string&, string&);
        string getnext(string, string*);
};

VocabList::VocabList() : vector<Category>(5){}

void VocabList::addToList(string word, string definition, int score, int category){
    VocabWord *vw;
    int cat;

    if (category == -1){
        if (this->contains(word)){
            if (this->lookup(word).compare(definition) != 0){
                cout << "Note: multiple definitions found for " << word << ":" << endl;
                cout << '\t' << this->lookup(word) << " || " << definition << endl;
            }
            return;
        }
        vw = new VocabWord(definition, DEFAULT_SCORE);
        cat = NEW;
    }else{
        if (this->contains(word)){
            cout << "Note: two copies of " << word;
            cout <<  " were saved. Discarding second copy." << endl;
            return;
        }
        vw = new VocabWord(definition, score);
        cat = category;
    }
    if (cat < this->size()) (*this)[cat][word] = vw;
}

bool VocabList::contains(string& word){
    for (Category category : *this){
        if (category.count(word) != 0) return true;
    }
    return false;
}

string VocabList::lookup(string word){
    for (Category category : *this){
        if (category.count(word) != 0) return category[word]->definition;
    }
    return "";
}
string VocabList::getnext(string line, string* newstr){
    int delim = line.find("||");
    if (delim >= 0){
            *newstr = line.substr(0, delim);
            return line.substr(delim + 2);
    }else{
        delim = line.find('|');
        *newstr = line.substr(0, delim);
        return line.substr(delim + 1);
    }
}

void VocabList::loadSavedList(string filename){
    string line, word, definition, score_str;
    ifstream inputfile('.' + filename + ".voc");
    if (inputfile.is_open()){
        while(getline(inputfile, line)){
            line = getnext(line, &word);
            line = getnext(line, &definition);
            line = getnext(line, &score_str);
            addToList(word, definition, stoi(score_str), stoi(line));
        }
    } 
}

void VocabList::loadNewWords(string filename){
    string line, word, definition;
    ifstream inputfile(filename + ".txt");
    if (inputfile.is_open()){
        while(getline(inputfile, line)){
            if (line.size()){
                //TODO: strip spaces
                if (word.size() == 0) word = line;
                else definition = line;
            }else{
                this->addToList(word, definition);
                word = "";
                definition = "";
            }
        }
        if (word.size()){
            this->addToList(word, definition);
        }
        inputfile.close();
    }else{
        cout << "No file named " << filename << ".txt found\n" << endl;
    }
}

void VocabList::printAll(){
    for (int i = 0; i < this->size(); i++){
        cout << "\nCategory " << i << endl;
        (*this)[i].print(i);
    }
    cout << endl;
}

void VocabList::saveToFile(string filename){
    ofstream file("." + filename + ".voc");
    for (int i = 0; i < this->size(); i++){
        (*this)[i].print(i, file);
    }
    file.close();
}

/*-------------------Session------------------------------*/

class Session{
    public:
        Session(string);
        string filename;
        VocabList vocablist;
        void round(bool);
    private:
        Category makeStudyList();
        int addFromCat(Category&, int, int);
        void categorize(string, VocabWord*);
        int getCategory(int);
        void quiz(Category&, bool);
};

Session::Session(string filename){
    this->filename = filename;
    vocablist.loadSavedList(filename);
    vocablist.loadNewWords(filename);
}

void Session::categorize(string key, VocabWord* word){
    int cat = getCategory(word->score);
    vocablist[cat][key] = word;
}

int Session::getCategory(int score){
    if (score < HARD_UPPERBOUND) return HARD;
    if (score < LEARN_UPPERBOUND) return FAMILIAR;
    if (score < MAX_SCORE) return REVIEW;
    return LEARNED;
}

int Session::addFromCat(Category& curlist, int to_add, int cat){
    //TODO: make random
    int added = 0;
    Category::iterator it;
    to_add = min(to_add, (int)vocablist[cat].size());
    for (added = 0; added < to_add; added++){
        it = vocablist[cat].begin();
        advance(it, added);
        curlist[it->first] = it->second;
    }
    for (auto word : curlist){
        vocablist[cat].erase(word.first);
    }
    return added;
}

Category Session::makeStudyList(){
    //TODO: replace numbers
    Category currentList;
    int toadd = WORDS_PER_ROUND;
    toadd -= addFromCat(currentList, toadd, HARD);
    if (toadd > 0) toadd -= addFromCat(currentList, min(toadd, 2), FAMILIAR);
    if (toadd > 0) toadd -= addFromCat(currentList, min(toadd, 2),NEW);
    addFromCat(currentList, 2, REVIEW);
    return currentList;
}

void Session::round(bool reverse = false){
    Category currentList = makeStudyList();
    quiz(currentList, reverse);
    this->vocablist.saveToFile(filename);
}

bool isCorrect(string response, string answer){
    return response.compare(answer) == 0;
}

void Session::quiz(Category& currentList, bool reverse){
    string response, answer, choice;
    bool correct;
    auto it = currentList.begin();

    while(it != currentList.end()){
        auto word = it;
        cout << (reverse ? word->second->definition : word->first) << ": ";
        getline(cin, response);
        answer = reverse ? word->first : word->second->definition;
        it++;
        switch(isCorrect(response, answer)){
            case false:
                cout << (reverse ? word->first : word->second->definition) << endl;
                cout << "\"O\" to override ";
                getline(cin, choice);
                if (choice.compare("O")) break;
            case true:
                cout << "Correct!" << endl;
                this->categorize(word->first, word->second);
                currentList.erase(word->first);
                cout << endl;
        }
    }
    if (currentList.size()) quiz(currentList, reverse);
}