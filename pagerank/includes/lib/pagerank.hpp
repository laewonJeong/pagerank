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

#define df 0.85
#define buf_size1 104857*4

using namespace std;
class Pagerank{
    public:
       void create_graph_data(string path);
       void initial_pagerank_value();
       static void calc_pagerank_value(int start, int end, double x, double y);
       static void thread_calc_pr(int index, double x, double y);
       static void combine_pr();
       static void thread_combine_pr(int i);
       void send_recv_pagerank_value(int start, int end);
       void run_pagerank(int iter);
       string max_pr();
       void init_connection(const char* ip, string server[], 
                            int number_of_server, int Port);
       void print_pr(); 
    private:
        vector<vector<size_t>> graph;
        vector<vector<size_t>> outgoing;
        vector<long double> pr;
        vector<long double> new_pr;
        vector<long double> my_pr;
        vector<string> pr1;
        vector<string> new_pr1;
        int num_of_vertex;
        int num_of_server;
        char send_buffer[4][buf_size1];
        char recv_buffer[4][buf_size1];
        int start1;
        int end1;
        string message;
        vector<int> num_outgoing;
        double diff;
        bool add_arc(size_t from, size_t to);
        template <class Vector, class T> bool insert_into_vector(Vector& v,
                                                             const T& t);
};