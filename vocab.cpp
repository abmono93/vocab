#include <unistd.h>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <algorithm>
#include <random>

//Category indices
#define NEW      0
#define HARD     1
#define FAMILIAR 2
#define REVIEW   3
#define LEARNED  4

//Score boundaries
#define DEFAULT_SCORE    8
#define HARD_UPPERBOUND  8
#define LEARN_UPPERBOUND 11
#define MAX_SCORE        15

//Words each round
#define WORDS_PER_ROUND  15
#define MAX_NEW_WORDS    3
#define MIN_REVIEW_WORDS 1
#define LEARNING_RATIO   1000

#define CORRECT   1
#define INCORRECT 0

using namespace std;

namespace util {

void strip_spaces(string& str){
	int double_spaces;
	while (isspace(str[str.size() - 1])) str.erase(str.end() - 1);
	while (isspace(str[0])) str.erase(str.begin());
	while ((double_spaces = str.find("  ")) >= 0) str.erase(double_spaces, 1);
}

string getnext(string& line, string* newstr){
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

vector<int> generate_indices(int num, bool randomize){
    vector<int> indices(num);

    for (int i = 0; i < num; i++) indices[i] = i;
    if (randomize) shuffle(indices.begin(), indices.end(), default_random_engine(time(NULL)));

    return indices;
}

vector<string> tokenize(string separated, char by){
    vector<string> ans;
    string next_word;
    stringstream tokens(separated);

    while(getline(tokens, next_word, by)){
        strip_spaces(next_word);
        ans.push_back(next_word);
    }

    return ans;
}

} // namespace util

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
	if (score == 999) this->score = LEARN_UPPERBOUND + 1;
    if (correct) {
        this->score = min(MAX_SCORE, score + 1);
        if (this->score == MAX_SCORE) cout << "Word learned!" << endl;
    }else{
        if (score > LEARN_UPPERBOUND) score = LEARN_UPPERBOUND;
        this->score = max(0, score - 1);
    }
}

/*-------------------Category------------------------------*/

struct Category : public map<string, VocabWord*>{
    ~Category();
    void print(int, ostream& dest = cout);
    pair<string, VocabWord*> at_index(int);
};

Category::~Category(){
    for (auto it=this->begin(); it != this->end(); it++){
        delete it->second;
    } 
}

void Category::print(int categoryNum, ostream& dest){
    for (auto it=this->begin(); it != this->end(); it++){
        dest << it->first << "||";
        dest << it->second->definition << "||";
        dest << it->second->score << "|";
        dest << categoryNum;
        dest << endl;
    }
}

pair<string, VocabWord*> Category::at_index(int i){
    Category::iterator it = this->begin();
    advance(it, i);
    return *it;
}

/*-------------------VocabList------------------------------*/

class VocabList : public vector<Category>{
    public:
        VocabList();
        void loadSavedScores(string&);
        void loadWords(string&);
        void saveToFile(string&);
        pair<string, VocabWord*> nextNewWord();
    private:
        map<string, int> newOrder;
        string lookup(string&);
        void redefine(string&, string&);
        void printAll();
        bool contains(string&);
        void addToList(string&, string&, int score = -1, int category = -1);
};

VocabList::VocabList() : vector<Category>(5){}

void VocabList::redefine(string& word, string& definition){
    for (int i = 0; i < this->size(); i++){
        if ((*this)[i].count(word) != 0) {
            (*this)[i][word]->definition = definition;
            return;
        }
    }
}

void VocabList::addToList(string& word, string& definition, int score, int category){
    VocabWord *vw;
    int cat;

    if (category == -1){
        string current = this->lookup(word);
        if (current != ""){
            cout << "Note: Two copies found for " << word << "- ";
            cout << current << " | " << definition << endl;
            cout << "\t Using second definition " << definition << endl;
            if (current.compare(definition) != 0) redefine(word, definition);
            return;
        }
        vw = new VocabWord(definition, DEFAULT_SCORE);
        cat = NEW;
        newOrder[word] = newOrder.size();
    }else{
        vw = (*this)[NEW][word];
        (*this)[NEW].erase(word);
        vw->score = score;
        cat = category;
        if (cat != NEW) newOrder.erase(word);
    }
    if (cat < this->size()) (*this)[cat][word] = vw;
}

bool VocabList::contains(string& word){
    string entry = lookup(word);
    return entry.compare("") != 0;
}

string VocabList::lookup(string& word){
    for (int i = 0; i < this->size(); i++){
        if ((*this)[i].count(word) != 0) return (*this)[i][word]->definition;
    }
    return "";
}

void VocabList::loadSavedScores(string& filename){
    string line, word, definition, score_str;
    ifstream inputfile('.' + filename + ".voc");

    if (inputfile.is_open()){
        while(getline(inputfile, line)){
            line = util::getnext(line, &word);
            line = util::getnext(line, &definition);
            line = util::getnext(line, &score_str);
            if ((*this)[NEW].count(word)) addToList(word, definition, stoi(score_str), stoi(line));
            else cout << "Deleting word - " <<  word << ": " << definition << endl;
        }
    } 
}

void VocabList::loadWords(string& filename){
    string line, word, definition;
    ifstream inputfile(filename + ".txt");

    if (inputfile.is_open()){
        while(getline(inputfile, line)){
            if (line.size()){
                util::strip_spaces(line);
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

pair<string, VocabWord*> VocabList::nextNewWord(){
    int order = 0;
    int max_order = 0;
    pair<string, VocabWord*> newest_word;

    for (auto new_word : (*this)[NEW]){
        order = newOrder[new_word.first];
        if (order > max_order){
            newest_word = new_word;
            max_order = order;
        }
    }
    this->newOrder.erase(newest_word.first);
    (*this)[NEW].erase(newest_word.first);

    return newest_word;
}

void VocabList::printAll(){
    for (int i = 0; i < this->size(); i++){
        cout << "\nCategory " << i << endl;
        (*this)[i].print(i);
    }
    cout << endl;
}

void VocabList::saveToFile(string& filename){
    ofstream file("." + filename + ".voc");
    for (int i = 0; i < this->size(); i++){
        (*this)[i].print(i, file);
    }
    file.close();
}

/*-------------------Session------------------------------*/

struct Result{
    Result(bool, bool);
    bool is_correct;
    bool is_exact;
};

Result::Result(bool is_correct, bool is_exact){
    this->is_correct = is_correct;
    this->is_exact = is_exact;
};

class Session{
    public:
        Session(string);
        void round(bool);
        void showStats();
        void save();
    private:
        string filename;
        VocabList vocablist;
        Category studyList;
        bool reverse;

        void fillStudyList();
        void categorize(string, VocabWord*);
        void quiz();
        int fromCatToStudyList(int, int, bool randomize=true);
        int addNewWords(int);
        int getCategory(int);
        bool grade(pair<string, VocabWord*>&, string&);
        Result evaluateAnswer(string&, string&);
        bool compareSemicolonSplit(int, string&, string&);
        bool compareCommaSplit(int, string&, string&);
        bool is_same_list(vector<string>&,vector<string>&);
        bool are_all_in_list(vector<string>&,vector<string>&);
        bool response_in_answers(string&,vector<string>&);
};

Session::Session(string filename){
    this->filename = filename;
    vocablist.loadWords(filename);
    vocablist.loadSavedScores(filename);
}

void Session::round(bool ask_definition = false){
    reverse = ask_definition;
    showStats();
    fillStudyList();
    quiz();
    save();
}

void Session::showStats(){
    cout << "--------------------------------" << endl;
	int hard_size = vocablist[HARD].size(); 
	int new_size = vocablist[NEW].size();
	int learning_size = vocablist[FAMILIAR].size();
	int review_size = vocablist[REVIEW].size();
	int learned_size = vocablist[LEARNED].size();

	int unlearned_size = hard_size + new_size + learning_size;
	cout << unlearned_size << " word";
	if (unlearned_size != 1) cout << "s";
	cout << " to learn:" << endl;
	
	if (new_size){	
		cout << '\t' << new_size << " new word";
		if (new_size != 1) cout << "s";
		cout << endl;
	}

	if (hard_size){	
		cout << '\t' << hard_size  << " difficult word";
		if (hard_size != 1) cout << "s";
		cout << endl;
	}

	if (learning_size){	
		cout << '\t' << learning_size  << " familiar word";
		if (learning_size != 1) cout << "s";
		cout << endl;
	}

	cout << review_size  << " word";
	if (review_size != 1) cout << "s";
	cout << " to review"  << endl;

    cout << learned_size  << " word";
	if (learned_size != 1) cout << "s";
	cout << " learned"  << endl;
	cout << "--------------------------------" << endl;
}

void Session::save(){
    this->vocablist.saveToFile(filename);
}

void Session::fillStudyList(){
    int toadd = WORDS_PER_ROUND;
    int min_familiar = pow(vocablist[FAMILIAR].size(), 2) / LEARNING_RATIO;
	toadd -= fromCatToStudyList(toadd, HARD);
    toadd -= fromCatToStudyList(min(toadd, min_familiar), FAMILIAR);
    toadd -= addNewWords(min(toadd, MAX_NEW_WORDS));
    toadd -= fromCatToStudyList(toadd, FAMILIAR);
	fromCatToStudyList(toadd + MIN_REVIEW_WORDS, REVIEW);
}

int Session::fromCatToStudyList(int to_add, int cat, bool randomize){
    int added;
    pair<string, VocabWord*> word;
    int num_items = vocablist[cat].size();
    vector<int> indices = util::generate_indices(num_items, randomize);

    to_add = min(to_add, num_items);
    for (added = 0; added < to_add; added++){
        word = vocablist[cat].at_index(indices[added]);
        studyList[word.first] = word.second;
    }
    for (auto word : studyList){
        if (vocablist[cat].count(word.first)) vocablist[cat].erase(word.first);
    }

    return added;
}

int Session::addNewWords(int to_add){
    int added = 0;
    pair<string, VocabWord*> word;

    to_add = min(to_add, (int)vocablist[NEW].size());
    while (added < to_add){
        word = vocablist.nextNewWord();
        studyList[word.first] = word.second;
        added++;
    }

    return added;
}

void Session::quiz(){
    string response;
    pair<string, VocabWord*> word;
    vector<pair<string, VocabWord*> > to_delete;
    vector<int> indices = util::generate_indices(studyList.size(), true);
    bool correct = false;

    cout << endl;
    for (int i = 0; i < indices.size(); i++){
        word = studyList.at_index(indices[i]);
        cout << i + 1  << ") ";
        cout << (reverse ? word.second->definition : word.first) << endl;
        getline(cin, response);
        correct = grade(word, response);
        if (correct) to_delete.push_back(word);
    }
    for (pair<string, VocabWord*> word : to_delete){
        this->categorize(word.first, word.second);
        studyList.erase(word.first);
    }
    if (studyList.size()){
        cout << studyList.size() << " to try again:" << endl;
        quiz();
    }
}

bool Session::grade(pair<string, VocabWord*>& word, string& response){
    string choice;
    string answer = reverse ? word.first : word.second->definition;

    Result result = evaluateAnswer(response, answer);
    if (!result.is_correct){
        cout << answer << endl;
        cout << "\"O\" to override ";
        getline(cin, choice);
        if (choice.compare("O")) {
            word.second->changeScore(INCORRECT);
            cout << endl; 
            return false;
        }
    }

    if (result.is_correct && !result.is_exact) cout << answer << endl;
    cout << "Correct!" << endl;
    word.second->changeScore(CORRECT);
    cout << endl;
    
    return true;
}

Result Session::evaluateAnswer(string& response, string& answer){
    bool correct = false;
    bool exact = false;
    util::strip_spaces(response);
    if (response.compare(answer) == 0) {
        correct = true;
        exact = true;
    }
    if (!correct){
        string also_accepted = answer;
        int open_paren = also_accepted.find("(");
        if (open_paren > -1){
            also_accepted.erase(open_paren, also_accepted.find(")") - open_paren + 1);
            util::strip_spaces(also_accepted);
            correct = evaluateAnswer(response, also_accepted).is_correct;
        }

        if (!correct){
            int semicolons = count(answer.begin(), answer.end(), ';');
            if (semicolons) correct = compareSemicolonSplit(semicolons, response, also_accepted);

            if (!correct && !semicolons){
                int commas = count(answer.begin(), answer.end(), ',');
                if (commas) correct = compareCommaSplit(commas, response, also_accepted);
            }
        }
    }

    return Result(correct, exact);
}

bool Session::compareSemicolonSplit(int num_semicolons, string& response, string& answer){
    vector<string> answers = util::tokenize(answer, ';');
    vector<string> responses = util::tokenize(response, ';');

    return is_same_list(responses, answers);
}

bool Session::compareCommaSplit(int num_commas, string& response, string& answer){
    if (response.rfind("to ", 2) == 0) response.erase(0, 3);
    if (answer.rfind("to ", 2) == 0) answer.erase(0, 3);
    vector<string> answers = util::tokenize(answer, ',');
    vector<string> responses = util::tokenize(response, ',');

    if (responses.size() == 0) return false;
    return are_all_in_list(responses, answers);
}

void Session::categorize(string key, VocabWord* word){
    int cat = getCategory(word->score);
    vocablist[cat][key] = word;
}

int Session::getCategory(int score){
    if (score <= HARD_UPPERBOUND) return HARD;
    if (score <= LEARN_UPPERBOUND) return FAMILIAR;
    if (score < MAX_SCORE) return REVIEW;
    return LEARNED;
}

bool Session::response_in_answers(string& next_response,vector<string>& answers){

    for (string next_answer : answers){
        if (evaluateAnswer(next_response, next_answer).is_correct){
            return true;
        }
    }

    return false;
}

bool Session::are_all_in_list(vector<string>& responses,vector<string>& answers){
    string next_response;
	stack<string, vector<string> > temp_stack(responses);
    bool next_response_in_list;

    while (temp_stack.size()){
        next_response = temp_stack.top();
        temp_stack.pop();
        if (!response_in_answers(next_response, answers)) return false;
    }

	return true;
}

bool Session::is_same_list(vector<string>& l1,vector<string>& l2){
    string str;
	stack<string, vector<string> > temp_stack(l1);
    int matches = 0;

	if (l1.size() != l2.size()) return false;
    while (temp_stack.size()){
        str = temp_stack.top();
        temp_stack.pop();
        for (string str2 : l2){
            if (evaluateAnswer(str, str2).is_correct){
                matches++;
                break;
            }
        }
    }

	return matches == l1.size();
}
