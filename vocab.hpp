#ifndef VOCAB_HPP
#define VOCAB_HPP
#include <string>
#include <vector>

#define DEFAULT_SCORE 8
#define CAT_HARD_UPPERBOUND 8
#define CAT_LEARN_UPPERBOUND 11
#define CAT_REVIEW_UPPERBOUND 14

#define MAX_NEW_PER_ROUND 5
#define LEARNING_RATIO 5

#define CAT_UNASSIGNED -1
#define CAT_HARD 0
#define CAT_NEW 1
#define CAT_LEARN 2
#define CAT_REVIEW 3
#define CAT_KNOW 4

#define TRUE 1
#define FALSE 0

#define STUDY_WORDS 20 
#define REVIEW_WORDS 2

#define DEFINITION 1
#define TARGET_LANG 2

#define CORRECT 1
#define INCORRECT -1

#define HIDDEN_FILE_PREFIX "."
#define VOCAB_FILE_EXTENSION ".voc"
#define TEXT_FILE_EXTENSION ".txt"

class Row{
	public:
		Row(std::string, std::string);
		int get_score();
		int get_category();
		int change_score(int);
		void set_score(int);
		void set_category(int);
		std::string get_word(int);

	private:
		int _score;
		int _category;
		std::string _target_lang;
		std::string _definition;
};

class Category : public std::vector<Row*>{
	public:
		Category();
		int contains(Row*);
		void print_category();
		
};

class VocabList{
	public:
		VocabList();
		void add_to_list(Row*);
		void categorize(Row*);
		void record_result(Row*, int);
		void read_from_file(std::string);
		void save_list(std::string);
		void load_saved_list(std::string);
		void set_target_language(std::string);
		std::string get_target_language();
		int index_of(std::string);
		Category rows;
		std::vector<Category *> difficulty_categories;
	private:
		std::string _target_language;
};

class Session{
	public:
		Session(std::string);
		void assemble_study_list();
		void round(int);
		void round_begin_message(int);
		void stats_message();
		void save();
		int is_correct(std::string, Row*, int);
		Category study_list;
	private:
		VocabList* _session_vocab_list;
		std::string _save_filename;
		void _clear_study_list();
};

#endif
