#pragma once

#define EXPORT __declspec (dllexport)

#include <vector>
#include <unordered_map>

class EXPORT A {

	public:

		A();
		~A();

		void Add(int x);
		void Print();

	private:

		std::vector<int> v;
		std::unordered_map<int, int> m;
		std::pair<int, int> p;

};
