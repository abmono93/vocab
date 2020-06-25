#include "vocab.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdlib.h>
#include <time.h>

using namespace std;

void strip_spaces(string& str){
//Removes leading and trailing spaces from str, makes all internal spaces exactly 1
	int double_spaces;
	while (isspace(str[str.size() - 1])) str.erase(str.end() - 1);
	while (isspace(str[0])) str.erase(str.begin());
	while ((double_spaces = str.find("  ")) >= 0) str.erase(double_spaces, 1);
}

int index_in(vector<string>& list, string test_item){
//Returns index of test_item in list, or -1 if it is not present
	for (int i = 0; i < list.size(); i++){
		if (list[i].compare(test_item) == 0) return i;
	}
	return -1;
}

int random_number(int i){return rand()%i;}

void shuffle_indices(vector<int>& indices, int desired_size){
//Populates indices with numbers zero through desired_size in a random order
	srand(time(NULL));
	while (indices.size() > 0) indices.erase(indices.begin());
	for (int i = 0; i < desired_size; i++) indices.push_back(i);
	random_shuffle(indices.begin(), indices.end(), random_number);
}

bool is_same_list(vector<string>& l1,vector<string>& l2){
//Returns TRUE if l1 and l2 contain the exact same set of strings
	int element;
	vector<string> temp_list;
	if (l1.size() != l2.size()) return false;
	for (int i = 0; i < l1.size(); i++){
		temp_list.push_back(l1[i]);
	}
	for (int i = 0; i < l1.size(); i++){
		element = index_in(temp_list, l2[i]);
		if (element >= 0) temp_list.erase(temp_list.begin() + element);
	}
	if (temp_list.size() == 0) return true;
	return false;
}

Row::Row(string target, string def){
	_score = DEFAULT_SCORE;
	_category = CAT_UNASSIGNED;
	_target_lang = target;
	_definition = def;
}

int Row::change_score(int result){
//Increases or decreases _score by one, depending if the result is CORRECT or INCORRECT
	if (result == CORRECT) {
		if (_score == 999) _score = CAT_LEARN_UPPERBOUND;
		_score += 1;
		return 1;
	}
	else if (result == INCORRECT) {
		if (_score > CAT_LEARN_UPPERBOUND) _score = CAT_LEARN_UPPERBOUND;
		if (_score > 0) _score -= 1;
		return 1;
	}
	return 0;
	
}

void Row::set_score(int s){
	if (s >= 0) _score = s;
}

void Row::set_category(int c){
	_category = c;
}

int Row::get_score(){
	return _score;
}

int Row::get_category(){
	return _category;
}

string Row::get_word(int choice){
//Returns the word in this row in either language
	if (choice == DEFINITION) return _definition;
	else if (choice == TARGET_LANG) return _target_lang;
	else return "error";
}

Category::Category(){}

int Category::contains(Row* testerRow){
//Returns the index of testerRow in the category, or -1 if it is not in this category
	int stop = this->size();
	if (stop == 0) return -1;
	for (int i = 0; i < stop; i++){
		if ((*this)[i] == testerRow) return i;
	}
	return -1;
}

void Category::print_category(){
//Prints each row in this category
	for (int i = 0; i < this->size(); i++){
		cout << (*this)[i]->get_word(TARGET_LANG) << " = " << (*this)[i]->get_word(DEFINITION) << endl;
	}
}
	
VocabList::VocabList(){
	difficulty_categories.push_back(new Category);
	difficulty_categories.push_back(new Category);
	difficulty_categories.push_back(new Category);
	difficulty_categories.push_back(new Category);
	difficulty_categories.push_back(new Category);
}

void VocabList::set_target_language(string lang){
	_target_language = lang;
}

string VocabList::get_target_language(){
	return _target_language;
}

void VocabList::add_to_list(Row new_word){
//Adds new word to VocabList and places it in the right category
	this->push_back(new_word);
	this->categorize(nullptr, this->size() - 1);
}

void VocabList::categorize(Row* this_row, int index_of_row){
//Places this_row in the right category
	int element, correct_cat;
	
	if (!this_row) this_row = &(*this)[index_of_row];
	int score = this_row->get_score();
	int cat = this_row->get_category();
	vector<Category*> d = difficulty_categories; //Just to save some typing
	if (score == DEFAULT_SCORE and cat == CAT_UNASSIGNED){
	//If it's a brand new row, add it to the category NEW and return
		this_row->set_category(CAT_NEW);
		d[CAT_NEW]->push_back(this_row);
		return;
	}
	//If it's marked new and still has default score, it hasn't been answered yet
	if (score == DEFAULT_SCORE and cat == CAT_NEW) correct_cat = CAT_NEW;
	//Otherwise set correct_cat
	else if (score <= CAT_HARD_UPPERBOUND) correct_cat = CAT_HARD;
	else if (score <= CAT_LEARN_UPPERBOUND) correct_cat = CAT_LEARN;
	else if (score <= CAT_REVIEW_UPPERBOUND or score == 999) correct_cat = CAT_REVIEW;
	else correct_cat = CAT_KNOW;
	if (cat != correct_cat){
	//If correct_cat is the not the current category, move it to the right one
			if (cat != CAT_UNASSIGNED){
			//Unless the row is brand new, remove it from it's previous category first
				element = d[cat]->contains(this_row);
				if (element >=0) d[cat]->erase(d[cat]->begin() + element);
			}
			this_row->set_category(correct_cat);
			d[correct_cat]->push_back(this_row);
	}else{
	//In case cat was manually set, make sure that the category actually contains this_row
		element = d[cat]->contains(this_row);
		if (element == -1) d[cat]->push_back(this_row);
	}
}

void VocabList::record_result(Row* cur_row, int result){
//Increases or decreases the score according to result, then places it in the correct category
	int check = cur_row->change_score(result);
	if (check) categorize(cur_row);
}

int VocabList::index_of(string target_word){
//Returns index of the row defining targe_word in the VocabList, or -1 if it is not present
	for (int i = 0; i < this->size(); i++){
		if ((*this)[i].get_word(TARGET_LANG) == target_word) return i;
	}
	return -1;
}

void VocabList::read_from_file(string filename){
//Opens a file in format <target language>\n<definition>\n\n and adds any new words
//in that file to this VocabList's rows
	int index;
	ifstream in(filename + TEXT_FILE_EXTENSION);
	string str, target, def;
	while (getline(in, str)){
	//Read line by line
		if (str.compare("") != 0){
		//If this line isn't empty, it should either be set to def or target
			strip_spaces(str);
			//If target hasn't been set yet, this is in the target lang (by convention)
			if (target.compare("") == 0) target = str;
			//If it has, this is the definition
			else def = str;
		}else{
		//If this line is empty, that means target and def were set to a corresponding pair
			Row* next = new Row(target, def);
			//Add it to the VocabList if it isn't already in there
			if ((index = index_of(target)) == -1 and target.compare("") != 0) add_to_list(*next);	
			else{
			//If it is already in there, print a message noting the duplicate and move on
				string previous_def = (*this)[index].get_word(DEFINITION);
				if (previous_def.compare(def) != 0){
					cout << "Note two definitions for " << target << " found: ";
					cout << previous_def << "/" << def << endl;
				}
			}
			//Reset the def and target fields for the next row
			target = "";
			def = "";
		}
	}
	in.close();
		
}

void VocabList::save_list(string filename){
//Saves VocabList in a file called <filename.voc> for future use in format:
//<target language>==<definition>++<score>::<category>
	ofstream outputfile;
	//Row* cur;
	filename = HIDDEN_FILE_PREFIX + filename + VOCAB_FILE_EXTENSION;
	outputfile.open(filename);
	outputfile << "tl=" << _target_language << endl;
	for (int i = 0; i < this->size(); i++){
		Row cur = (*this)[i];
		outputfile << cur.get_word(TARGET_LANG) << "==" << cur.get_word(DEFINITION) << "++" << cur.get_score() << "::" << cur.get_category() << endl;
	}
	outputfile.close();
		
}

void VocabList::load_saved_list(string filename){
//Reads from file written by save_list and adds those words to this VocabList
	int double_equals, double_plus, double_colon;
	string str, target, def, score, category, target_language;
	filename = HIDDEN_FILE_PREFIX + filename + VOCAB_FILE_EXTENSION;
	ifstream inputfile(filename);
	if (!inputfile.is_open()) return;
	//First line has target language, so grab that and set it
	getline(inputfile, str);
	target_language = str.substr(3, str.size());
	set_target_language(target_language);
	while (getline(inputfile, str)){
		//Look for indices in str where delimiters are found
		double_equals = str.find("==");
		if (double_equals == -1) break;
		double_plus = str.find("++");
		if (double_plus == -1) break;
		double_colon = str.find("::");
		if (double_colon == -1) break;
		//Each relevant string is between two delimiters
		target = str.substr(0, double_equals);
		def = str.substr(double_equals + 2, double_plus - double_equals - 2);
		score = str.substr(double_plus + 2, double_colon - double_plus - 2);
		//Except category which is after the last one
		category = str.substr(double_colon + 2, str.size() - double_colon - 2);
		//Create a new row using the strings found above, and set score and category
		Row* next = new Row(target, def);
		next->set_score(stoi(score));
		next->set_category(stoi(category));
		//Add it to this VocabList if it's not already in there
		if (index_of(target) == -1) add_to_list(*next);
	}
	inputfile.close();
}

Session::Session(string filename){
	_session_vocab_list = new VocabList();
	_session_vocab_list->load_saved_list(filename);
	_session_vocab_list->read_from_file(filename);
	_save_filename = filename;
}

void Session::save(){
	_session_vocab_list->save_list(_save_filename);
}

bool Session::is_correct(string user_response, Row* cur_row, int answer_with){
//Returns TRUE if the user response counts as correct for this row
	int num_commas, ur_num_commas, num_semicolons, ur_num_semicolons, num_parens;
	//Look up the correct answer for this row
	string correct_answer = cur_row->get_word(answer_with);
	// If the user typed an exact match, answer is correct
	if (user_response.compare(correct_answer) == 0) return TRUE;
	num_parens = count(correct_answer.begin(), correct_answer.end(), ')');
	if (num_parens > 0){
	//It is correct if user did not type parentheses but was otherwise the same
		int open_paren = correct_answer.find("(");
		int close_paren = correct_answer.find(")");
		//Remove the parentheses from correct_answer so all subsequent tests ignore parentheses
		correct_answer.erase(open_paren, close_paren - open_paren + 1);
		strip_spaces(correct_answer);
		if (user_response.compare(correct_answer) == 0) {
			cout << cur_row->get_word(answer_with) << endl;
			return TRUE;
		}
	}
	num_commas = count(user_response.begin(), user_response.end(), ',');
	if (num_commas > 0){
	//If there are commas it is also possible they gave the correct definition in
	//a different order
		string next_word;
		string ur = user_response;
		//Put all the words entered by the user and the correct words in two lists
		vector <string> ur_words;
		vector <string> correct_words;
		//Strip "to " so that a verb definition will work
		//e.g. "to feel, experience" = "to experience, feel"
		string ignore_str = "to ";
		if (ur.rfind(ignore_str, 2) == 0) ur.erase(0, 3);
		if (correct_answer.rfind(ignore_str, 2) == 0) correct_answer.erase(0, 3);
		//Add each comma separated section (without spaces) to the appropriate list
		stringstream ur_tokenized(ur);
		while(getline(ur_tokenized, next_word, ',')){
			strip_spaces(next_word);
			ur_words.push_back(next_word);
		}
		stringstream correct_tokenized(correct_answer);
		while(getline(correct_tokenized, next_word, ',')){
			strip_spaces(next_word);
			correct_words.push_back(next_word);
		}
		//If the two lists are the same, it also counts as correct
		if (is_same_list(correct_words, ur_words)) {
			cout << cur_row->get_word(answer_with) << endl;
			return TRUE;
		}	

	}
	num_semicolons = count(user_response.begin(), user_response.end(), ';');
	if (num_semicolons > 0){
	//Same is true of semicolons, except "to " should be included, e.g. "to do; to make"
		string next_word;
		string ur = user_response;
		vector <string> ur_words;
		vector <string> correct_words;
		stringstream ur_tokenized(ur);
		while(getline(ur_tokenized, next_word, ';')){
			strip_spaces(next_word);
			ur_words.push_back(next_word);
		}
		stringstream correct_tokenized(correct_answer);
		while(getline(correct_tokenized, next_word, ';')){
			strip_spaces(next_word);
			correct_words.push_back(next_word);
		}
		if (is_same_list(correct_words, ur_words)){
			cout << cur_row->get_word(answer_with) << endl;
			return TRUE;
		}	

	}
	//Also check they didn't put extra spaces
	strip_spaces(user_response);
	if (user_response.compare(correct_answer) == 0) return true;
	return false;	
}

void Session::round_begin_message(int answer_with){
//Prints a message to begin the round
	stats_message();
	cout << endl << "Starting new round of quizzing. Answer with ";
	if (answer_with == TARGET_LANG) cout << _session_vocab_list->get_target_language() << endl;
	else if (answer_with == DEFINITION) cout << "definition" << endl;
	cout << endl;
}

void Session::stats_message(){
//Prints a message at the end of a round with some stats
	cout << "--------------------------------" << endl;
	int hard_size = _session_vocab_list->difficulty_categories[CAT_HARD]->size(); 
	int new_size = _session_vocab_list->difficulty_categories[CAT_NEW]->size();
	int learning_size = _session_vocab_list->difficulty_categories[CAT_LEARN]->size();
	int review_size = _session_vocab_list->difficulty_categories[CAT_REVIEW]->size();

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

void Session::round(int answer_with){	
//Performs one round of quizzing in one direction
	string user_response, choice;
	int ask, element, num_wrong;
	Row* cur_row;
	Category incorrect, correct_this_loop;
	vector<int> shuffled_indices;
	//Get a list of words to quiz on based on the categories
	assemble_study_list();
	round_begin_message(answer_with);
	//Set the question/response language based on answer_with argument passed
	if (answer_with == TARGET_LANG) ask = DEFINITION;
	else if (answer_with == DEFINITION) ask = TARGET_LANG;
	else return;
	//Use a shuffled version of the study list
	shuffle_indices(shuffled_indices, study_list.size()); 
	for (int i = 0; i < study_list.size(); i++){
	//Print the word being quizzed ang get a user response
		cur_row = study_list[shuffled_indices[i]];
		cout << i + 1 << ") " << cur_row->get_word(ask) << " = ";
		getline(cin, user_response);
		if (is_correct(user_response, cur_row, answer_with)) {
		//If the answer was correct tell them so, and record the fact they got it right
			cout << "Correct!" << endl;
			_session_vocab_list->record_result(cur_row, CORRECT);
		}else{
		//If they were wrong show the correct answer and give them a chance to override
			cout << cur_row->get_word(answer_with) << endl;
			cout << "\"O\" to override ";
			getline(cin, choice);
			if (choice.compare("O") != 0){
			//If they don't override, record they got it wrong and keep this row in
			//the list incorrect for re-testing at the end
				 _session_vocab_list->record_result(cur_row, INCORRECT);
				incorrect.push_back(cur_row);
			}
			else _session_vocab_list->record_result(cur_row, CORRECT);
		}
		cout << endl;
	}
	while (incorrect.size() > 0){
	//Keep re-testing the ones they got wrong until they get them all right
		num_wrong = incorrect.size();
		//Shuffle them each time and print how many they are being re-tested on
		shuffle_indices(shuffled_indices, num_wrong);
		cout << num_wrong << " to try again:" << endl << endl;
		for (int i = 0; i < num_wrong; i++){
		//Repeat as above (except save the ones they got right instead of the ones they got wrong)
			cur_row = incorrect[shuffled_indices[i]];
			cout << i + 1 << ") " << cur_row->get_word(ask) << " = ";
			getline(cin, user_response);
			if (is_correct(user_response, cur_row, answer_with)){
				cout << "Correct!" << endl;
				_session_vocab_list->record_result(cur_row, CORRECT);
				correct_this_loop.push_back(cur_row);
			}else{
				cout << cur_row->get_word(answer_with) << endl;
				cout << "\"O\" to override ";
				getline(cin, choice);
				if (choice.compare("O") != 0) _session_vocab_list->record_result(cur_row, INCORRECT);
				else {
					correct_this_loop.push_back(cur_row);
					_session_vocab_list->record_result(cur_row, CORRECT);
				}
			}
			cout << endl;
		}
		while (correct_this_loop.size() > 0){
		//Remove any words they got right this loop from incorrect
			element = incorrect.contains(correct_this_loop[0]);
			incorrect.erase(incorrect.begin() + element);
			correct_this_loop.erase(correct_this_loop.begin());
		}
	}
	//Save the list to be read next time the program starts up
	_session_vocab_list->save_list(_save_filename);
}

void Session::_clear_study_list(){
//Removes items from study list until there are none left
	int stop = study_list.size();
	for (int i = 0; i < stop; i++) study_list.erase(study_list.begin());
}

void Session::assemble_study_list(){
//Assembles a list of words to study for one round of quizzing
	int learn_index, can_add, size_of_to_review, size_of_learn;
	Category *cat_hard, *cat_learn, *cat_new, *cat_review;
	vector<int> shuffled_indices;
	//Keep track of how many spaces are left to add words to
	int to_add = STUDY_WORDS;
	//Make sure the current study list is empty to start
	_clear_study_list();

	cat_hard = _session_vocab_list->difficulty_categories[CAT_HARD];
	can_add = min(to_add, (int) cat_hard->size());
	for (int i = 0; i < can_add; i++){
	//First fill as many spaces as possible with hard words
		study_list.push_back((*cat_hard)[i]);
		to_add--;
	}

	cat_learn = _session_vocab_list->difficulty_categories[CAT_LEARN];
	size_of_learn = (int) cat_learn->size();
	shuffle_indices(shuffled_indices, size_of_learn);
	if (to_add > 0){
	//Next add a few of the familiar words at random
		can_add = min(to_add, size_of_learn);
		can_add = min(can_add, size_of_learn * size_of_learn / LEARNING_RATIO);
		for (learn_index = 0; learn_index < can_add; learn_index++) {
			study_list.push_back((*cat_learn)[shuffled_indices[learn_index]]);
			to_add--;
		}
	}

	cat_new = _session_vocab_list->difficulty_categories[CAT_NEW];
	if (to_add > 0){
	//Only put a few new words in at a time
		can_add = min(to_add, (int) cat_new->size());
		can_add = min(can_add, MAX_NEW_PER_ROUND);
		for (int i = 0; i < can_add; i++){
			study_list.push_back((*cat_new)[i]);
			to_add--;
		}
	}
	if (to_add > 0){
	//If there's space left, fill it with more random familiar words
		can_add = min(to_add, size_of_learn - learn_index);
		can_add += learn_index;
		for (learn_index = learn_index; learn_index < can_add; learn_index++){
			study_list.push_back((*cat_learn)[shuffled_indices[learn_index]]);
			to_add--;
		}
	}
	
	//Fill the rest of the spots with random words to reveiw (at least 2)
	cat_review = _session_vocab_list->difficulty_categories[CAT_REVIEW];
	size_of_to_review = (int) cat_review->size();
	//to_add = max(REVIEW_WORDS, to_add);
	to_add += REVIEW_WORDS;
	can_add = min(to_add, size_of_to_review);
	shuffle_indices(shuffled_indices, size_of_to_review);
	for (int i = 0; i < can_add; i++) study_list.push_back((*cat_review)[shuffled_indices[i]]);
}
