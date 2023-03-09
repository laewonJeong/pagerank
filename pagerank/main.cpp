#include<time.h>
#include "pagerank.hpp"
#include "myRDMA.hpp"
#include "tcp.hpp"
#include <map>

#define num_of_node 3
#define port 40145
#define server_ip "192.168.0.100"
#define iter 100000
string node1[num_of_node] = {server_ip,"192.168.0.101","192.168.0.103"};//,"192.168.0.104"};//,"192.168.0.106"};//,"192.168.1.102","192.168.1.103"};
using namespace std;
//using namespace stdext;

bool is_server(string ip){
  if(ip == server_ip)
    return true;
  return false;
}

int main(int argc, char* argv[]){
    Pagerank pagerank;
    TCP tcp;
    string data_path = argv[1];
 
    string my_ip = tcp.check_my_ip();
    cout<<"my ip is " << my_ip << endl;

    pagerank.create_graph_data(data_path, argv[2]);
    int num_of_vertex = pagerank.get_num_of_vertex();

    pagerank.init_connection(my_ip.c_str(),node1,num_of_node,port,num_of_vertex);
    
    cout << "--------------------------------------------------------" << endl;
    
    pagerank.initial_pagerank_value();

    struct timespec begin, end ;
    clock_gettime(CLOCK_MONOTONIC, &begin);

    pagerank.run_pagerank(iter);

    clock_gettime(CLOCK_MONOTONIC, &end);
    long double time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0;
    
    if(!is_server(my_ip)){
        pagerank.print_pr();
        cout << "Done." << endl;
        cout << "-------------------------------------" <<endl;
        printf("수행시간: %Lfs.\n", time);

        // important thing
        cout << "-------------------------------------" <<endl;
        cout << pagerank.max_pr() << endl;
        cout << "-------------------------------------" <<endl;
    }
    
}