#include"remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
	std::set<std::set<std::string>> vectors;
	std::vector<int> deleted;

	for (auto i = search_server.begin(); i != search_server.end(); i++) {
		std::map<std::string, double> move = search_server.GetWordFrequencies(*i);
		std::set<std::string> move_str;
		for (auto i : move) {
			move_str.insert(i.first);
		}
		if (vectors.count(move_str)) {
			deleted.push_back(*i);
		}
		else {
			vectors.insert(move_str);
		}

	}

	for (int i : deleted) {
		std::cout << "Found duplicate document id " << i << std::endl;
		search_server.RemoveDocument(i);
	}
}