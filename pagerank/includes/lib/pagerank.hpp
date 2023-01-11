#include <iostream>
#include <map>
#include <vector>
#include <stdio.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <thread>

#define df 0.85

using namespace std;
class Pagerank{
    public:
       void create_graph_data(string path, int num_of_vertex);
       void initial_pagerank_value();
       static void calc_pagerank_value();
       static void thread_calc_pr(int index);
       void change_pagerank_value();
       void run_pagerank(int iter);
       int max_pr();
    private:
        vector<int> graph[4039];
        vector<double> pr;
        vector<double> new_pr;
        int num_of_vertex;
};