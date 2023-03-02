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
#include <stdlib.h>
#include <math.h>
#include <unordered_map>

#define df 0.85
#define buf_size1 1048676*30

using namespace std;
class Pagerank{
    public:
       void create_graph_data(string path, string del);
       void initial_pagerank_value();
       static void calc_pagerank_value(int start, int end, double x, double y);
       static void thread_calc_pr(int index, double x, double y);
       static void combine_pr();
       static void thread_combine_pr(int i);
       void send_recv_pagerank_value(int start, int end);
       void run_pagerank(int iter);
       void gather_pagerank(string opcode, int i, vector<long double> pr);
       void scatter_pagerank(string opcode, int i, vector<long double> pr);
       string max_pr();
       void init_connection(const char* ip, string server[], 
                            int number_of_server, int Port, int num_of_vertex);
       void print_pr();
       int get_num_of_vertex(); 
    private:
        vector<vector<size_t>> graph;
        unordered_map<size_t, vector<size_t>> graph1;
        unordered_map<string, double> pr1;
        unordered_map<string, double> new_pr1;
        vector<vector<size_t>> outgoing;
        vector<long double> pr;
        vector<long double> new_pr;
        vector<long double> my_pr;
        string my_ip;
        int num_of_vertex;
        int num_of_server;
        int start1;
        int end1;
        string message;
        vector<int> num_outgoing;
        double diff;
        bool add_arc(size_t from, size_t to);
        template <class Vector, class T> bool insert_into_vector(Vector& v,
                                                             const T& t);
};