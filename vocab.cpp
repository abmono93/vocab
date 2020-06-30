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
#define MAX_SCORE 14

//Words each round
#define WORDS_PER_ROUND 10
#define MAX_NEW_WORDS 5
#define LEARNING_RATIO 750

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
    if (correct){
		if (score == 999) this->score = LEARN_UPPERBOUND;
        this->score = min(MAX_SCORE, score + 1);
    }else{
        if (score > LEARN_UPPERBOUND) score = LEARN_UPPERBOUND;
        this->score = max(0, score - 1);
    }
}

/*-------------------Category------------------------------*/

struct Category : public map<string, VocabWord*>{
    void print(int, ostream& dest = cout);
    pair<string, VocabWord*> at_index(int);
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
    private:
        void addToList(string&, string&, int score = -1, int category = -1);
        int categorize(int);
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
    for (Category category : *this){
        if (category.count(word) != 0) return category[word]->definition;
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
};

Session::Session(string filename){
    this->filename = filename;
    vocablist.loadSavedList(filename);
    vocablist.loadNewWords(filename);
}

void Session::save(){
    this->vocablist.saveToFile(filename);
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

void Session::categorize(string key, VocabWord* word){
    int cat = getCategory(word->score);
    vocablist[cat][key] = word;
}

int Session::getCategory(int score){
    if (score <= HARD_UPPERBOUND) return HARD;
    if (score <= LEARN_UPPERBOUND) return FAMILIAR;
    if (score <= MAX_SCORE) return REVIEW;
    return LEARNED;
}

vector<int> generateIndices(int num){
    vector<int> indices(num);
    for (int i = 0; i < num; i++) indices[i] = i;
    shuffle(indices.begin(), indices.end(), default_random_engine(time(NULL)));
    return indices;
}

int Session::fromCatToStudyList(int to_add, int cat){
    Category::iterator it;
    int added = 0;
    vector<int> indices = generateIndices(vocablist[cat].size());
    to_add = min(to_add, (int)vocablist[cat].size());
    for (added = 0; added < to_add; added++){
        auto word = vocablist[cat].at_index(indices[added]);
        studyList[word.first] = word.second;
    }
    for (auto word : studyList){
        if (vocablist[cat].count(word.first)) vocablist[cat].erase(word.first);
    }
    return added;
}

void Session::fillStudyList(){
    Category currentList;
    int toadd = WORDS_PER_ROUND;
    int min_familiar = pow(vocablist[FAMILIAR].size(), 2) / LEARNING_RATIO;
printf("min familiar = %d", min_familiar);
	toadd -= fromCatToStudyList(toadd, HARD);
    toadd -= fromCatToStudyList(min(toadd, min_familiar), FAMILIAR);
    toadd -= fromCatToStudyList(min(toadd, MAX_NEW_WORDS), NEW);
    toadd -= fromCatToStudyList(toadd, FAMILIAR);
	fromCatToStudyList(toadd, REVIEW);
    fromCatToStudyList(2, REVIEW);
}

void Session::round(bool reverse = false){
    showStats();
    fillStudyList();
    quiz(reverse);
    save();
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

bool is_same_list(vector<string>& l1,vector<string>& l2){
	if (l1.size() != l2.size()) return false;
	stack<string> temp_list;
    int matches = 0;

	for (string str : l1){
		temp_list.push(str);
	}
    while (temp_list.size()){
        string str = temp_list.top();
        temp_list.pop();
        for (string str2 : l2){
            if (str.compare(str2) == 0){
                matches++;
                break;
            }
        }
    }

	return matches == l1.size();
}

bool isSameSet(string set1, string set2, char delim){
    vector<string> firstSet = tokenize(set1, delim);
    vector<string> secondSet = tokenize(set2, delim);

    return is_same_list(firstSet, secondSet);
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

    int num_commas = count(response.begin(), response.end(), ',');
	if (num_commas > 0 && num_commas == count(also_accepted.begin(), also_accepted.end(), ',')){
        if (response.rfind("to ", 2) == 0) response.erase(0, 3);
        if (also_accepted.rfind("to ", 2) == 0) also_accepted.erase(0, 3);
        if (isSameSet(response, also_accepted, ',')){
			cout << answer << endl;
			return true;
		}
	}

    int num_semicolons = count(response.begin(), response.end(), ';');
	if (num_semicolons > 0 && num_semicolons == count(also_accepted.begin(), also_accepted.end(), ';')){
        if (isSameSet(response, also_accepted, ';')){
			cout << answer << endl;
			return true;
		}
	}

    return false;
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

void Session::quiz(bool reverse){
    pair<string, VocabWord*> word;
    string response;
    vector<pair<string, VocabWord*> > to_delete;
    vector<int> indices = generateIndices(studyList.size());
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
    for (auto word : to_delete){
        this->categorize(word.first, word.second);
        studyList.erase(word.first);
    }
    if (studyList.size()){
        cout << studyList.size() << " to try again:" << endl;
        quiz(reverse);
    }
}
