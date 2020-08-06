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
#define NEW 0
#define HARD 1
#define FAMILIAR 2
#define REVIEW 3
#define LEARNED 4

//Score boundaries
#define DEFAULT_SCORE 8
#define HARD_UPPERBOUND 8
#define LEARN_UPPERBOUND 11
#define MAX_SCORE 15

//Words each round
#define WORDS_PER_ROUND 10
#define MAX_NEW_WORDS 3
#define MIN_REVIEW_WORDS 2
#define LEARNING_RATIO 1000

#define CORRECT 1
#define INCORRECT 0

using namespace std;

void strip_spaces(string& str){
	int double_spaces;
	while (isspace(str[str.size() - 1])) str.erase(str.end() - 1);
	while (isspace(str[0])) str.erase(str.begin());
	while ((double_spaces = str.find("  ")) >= 0) str.erase(double_spaces, 1);
}


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
        void loadSavedList(string&);
        void loadNewWords(string&);
        void saveToFile(string&);
        void printAll();
        string lookup(string&);
        bool contains(string&);
        void addToList(string&, string&, int score = -1, int category = -1);
    private:
        string getnext(string&, string*);
};

VocabList::VocabList() : vector<Category>(5){}

void VocabList::addToList(string& word, string& definition, int score, int category){
    VocabWord *vw;
    int cat;

    if (category == -1){
        string current = this->lookup(word);
        if (current != ""){
            if (current.compare(definition) != 0){
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
    string entry = lookup(word);
    return entry.compare("") != 0;
}

string VocabList::lookup(string& word){
    for (int i = 0; i < this->size(); i++){
        if ((*this)[i].count(word) != 0) return (*this)[i][word]->definition;
    }
    return "";
}

string VocabList::getnext(string& line, string* newstr){
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

void VocabList::loadSavedList(string& filename){
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

void VocabList::loadNewWords(string& filename){
    string line, word, definition;
    ifstream inputfile(filename + ".txt");

    if (inputfile.is_open()){
        while(getline(inputfile, line)){
            if (line.size()){
                strip_spaces(line);
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

void VocabList::saveToFile(string& filename){
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
        void round(bool);
        void showStats();
        void save();
    private:
        string filename;
        VocabList vocablist;
        Category studyList;

        void fillStudyList();
        void categorize(string, VocabWord*);
        void quiz(bool);
        int fromCatToStudyList(int, int);
        int getCategory(int);
        bool grade(pair<string, VocabWord*>&, string&, bool);
        bool isCorrect(string&, string&);
        bool compareSemicolonSplit(int, string&, string&);
        bool compareCommaSplit(int, string&, string&);
        vector<string> tokenize(string , char);
        vector<int> generate_random_indices(int);
        bool is_same_list(vector<string>&,vector<string>&);
};

Session::Session(string filename){
    this->filename = filename;
    vocablist.loadSavedList(filename);
    vocablist.loadNewWords(filename);
}

void Session::round(bool reverse = false){
    showStats();
    fillStudyList();
    quiz(reverse);
    save();
}

void Session::showStats(){
    cout << "--------------------------------" << endl;
	int hard_size = vocablist[HARD].size(); 
	int new_size = vocablist[NEW].size();
	int learning_size = vocablist[FAMILIAR].size();
	int review_size = vocablist[REVIEW].size();

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
	cout << "--------------------------------" << endl;
}

void Session::save(){
    this->vocablist.saveToFile(filename);
}

void Session::fillStudyList(){
    int toadd = WORDS_PER_ROUND;
    int min_familiar = pow(vocablist[FAMILIAR].size(), 2) / LEARNING_RATIO;
    //cout << "min familiar = " << min_familiar << endl;
	toadd -= fromCatToStudyList(toadd, HARD);
    toadd -= fromCatToStudyList(min(toadd, min_familiar), FAMILIAR);
    toadd -= fromCatToStudyList(min(toadd, MAX_NEW_WORDS), NEW);
    toadd -= fromCatToStudyList(toadd, FAMILIAR);
	fromCatToStudyList(toadd + MIN_REVIEW_WORDS, REVIEW);
}

int Session::fromCatToStudyList(int to_add, int cat){
    pair<string, VocabWord*> word;
    Category::iterator it;
    int added = 0;
    vector<int> indices = generate_random_indices(vocablist[cat].size());

    to_add = min(to_add, (int)vocablist[cat].size());
    for (added = 0; added < to_add; added++){
        word = vocablist[cat].at_index(indices[added]);
        studyList[word.first] = word.second;
    }
    for (auto word : studyList){
        if (vocablist[cat].count(word.first)) vocablist[cat].erase(word.first);
    }

    return added;
}

void Session::quiz(bool reverse){
    string response;
    pair<string, VocabWord*> word;
    vector<pair<string, VocabWord*> > to_delete;
    vector<int> indices = generate_random_indices(studyList.size());
    bool correct = false;

    cout << endl;
    for (int i = 0; i < indices.size(); i++){
        word = studyList.at_index(indices[i]);
        cout << i + 1  << ") ";
        cout << (reverse ? word.second->definition : word.first) << ": ";
        getline(cin, response);
        correct = grade(word, response, reverse);
        if (correct) to_delete.push_back(word);
    }
    for (pair<string, VocabWord*> word : to_delete){
        this->categorize(word.first, word.second);
        studyList.erase(word.first);
    }
    if (studyList.size()){
        cout << studyList.size() << " to try again:" << endl;
        quiz(reverse);
    }
}

bool Session::grade(pair<string, VocabWord*>& word, string& response, bool reverse){
    string choice;
    string answer = reverse ? word.first : word.second->definition;

    if (!isCorrect(response, answer)){
        cout << (reverse ? word.first : word.second->definition) << endl;
        cout << "\"O\" to override ";
        getline(cin, choice);
        if (choice.compare("O")) {
            word.second->changeScore(INCORRECT);
            cout << endl; 
            return false;
        }
    }
    cout << "Correct!" << endl;
    word.second->changeScore(CORRECT);
    cout << endl;
    
    return true;
}

bool Session::isCorrect(string& response, string& answer){
    if (response.compare(answer) == 0) return true;
    string also_accepted = answer;

    int open_paren = answer.find("(");
	if (open_paren > -1){
		also_accepted.erase(open_paren, answer.find(")") - open_paren + 1);
        strip_spaces(also_accepted);
		if (isCorrect(response, also_accepted)){
			cout << answer << endl;
			return true;
		}
	}

    int semicolons = count(response.begin(), response.end(), ';');
    if (semicolons)  return compareSemicolonSplit(semicolons, response, also_accepted);

    int commas = count(response.begin(), response.end(), ',');
    if (commas) return compareCommaSplit(commas, response, also_accepted);

    return false;
}

bool Session::compareSemicolonSplit(int num_semicolons, string& response, string& answer){
    vector<string> answers = tokenize(answer, ';');
    vector<string> responses = tokenize(response, ';');

    return is_same_list(answers, responses);
}

bool Session::compareCommaSplit(int num_commas, string& response, string& answer){
    if (response.rfind("to ", 2) == 0) response.erase(0, 3);
    if (answer.rfind("to ", 2) == 0) answer.erase(0, 3);
    vector<string> answers = tokenize(answer, ',');
    vector<string> responses = tokenize(response, ',');

    return is_same_list(answers, responses);
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

vector<int> Session::generate_random_indices(int num){
    vector<int> indices(num);
    for (int i = 0; i < num; i++) indices[i] = i;
    shuffle(indices.begin(), indices.end(), default_random_engine(time(NULL)));
    return indices;
}

vector<string> Session::tokenize(string separated, char by){
    vector<string> ans;
    string next_word;
    stringstream tokens(separated);

    while(getline(tokens, next_word, by)){
        strip_spaces(next_word);
        ans.push_back(next_word);
    }

    return ans;
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
            if (isCorrect(str, str2)){
                matches++;
                break;
            }
        }
    }

	return matches == l1.size();
}