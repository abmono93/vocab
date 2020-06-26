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

using namespace std;

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
        score = min(MAX_SCORE, score + 1);
    }else{
        if (score > LEARN_UPPERBOUND) score = LEARN_UPPERBOUND;
        score = max(0, score - 1);
    }
}

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

class VocabList{
    public:
        vector<Category> categories;
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

VocabList::VocabList(){
    this->categories = vector<Category>(4);
}

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
    if (cat < categories.size()) categories[cat][word] = vw;
}

int VocabList::categorize(int score){
    if (score < HARD_UPPERBOUND) return HARD;
    if (score < LEARN_UPPERBOUND) return FAMILIAR;
    if (score < MAX_SCORE) return REVIEW;
    return LEARNED;
}

bool VocabList::contains(string& word){
    for (Category category : categories){
        if (category.count(word) != 0) return true;
    }
    return false;
}

string VocabList::lookup(string word){
    for (Category category : categories){
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
    for (int i = 0; i < categories.size(); i++){
        cout << "\nCategory " << i << endl;
        categories[i].print(i);
    }
    cout << endl;
}

void VocabList::saveToFile(string filename){
    ofstream file("." + filename + ".voc");
    for (int i = 0; i < categories.size(); i++){
        categories[i].print(i, file);
    }
    file.close();
}

class Session{
    public:
        Session(string);
        string filename;
        VocabList vocablist;
        void round(bool);
    private:
        Category makeStudyList(bool);

};

Session::Session(string filename){
    this->filename = filename;
    vocablist.loadSavedList(filename);
    vocablist.loadNewWords(filename);
}

Category Session::makeStudyList(bool answerWithDef){
    Category studylist;
}

void Session::round(bool answerWithDef){
    Category studylist = makeStudyList(answerWithDef);

}