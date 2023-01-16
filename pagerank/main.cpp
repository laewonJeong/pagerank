#include<time.h>
#include "pagerank.hpp"
#include "myRDMA.hpp"

#define port 40145
#define num_of_node 4
#define server_ip "192.168.1.100"
#define iter 100000

string node[num_of_node] = {server_ip,"192.168.1.101","192.168.1.102","192.168.1.103"};

using namespace std;
int main(int argc, char* argv[]){
    if(argc != 2){
        cerr << argv[0] << " <MY IP> " << endl;
        exit(1);
    }
    if(server_ip != node[0]){
        cerr << "node[0] is not server_ip" << endl;
        exit(1);
    }
    
    string data_path = "/home/lwjeong/git/pagerank/pagerank/facebook_data.txt";
    int graph_data_vertex = 4039;


    Pagerank pagerank;
    
    // init connection for rdma communication
    pagerank.init_connection(argv[1], node, num_of_node, port, graph_data_vertex);    

    // check time
    struct timespec begin, end ;
    clock_gettime(CLOCK_MONOTONIC, &begin);

    // Create graph data
    pagerank.create_graph_data(data_path, graph_data_vertex);

    // Initial pr value each vertex
    pagerank.initial_pagerank_value();

    //calc pagerank
    pagerank.run_pagerank(iter);

    // important thing
    int important = pagerank.max_pr();
    cout << "important page is " << important<< " page.\n" <<endl;

    // check time
    clock_gettime(CLOCK_MONOTONIC, &end);
    long double time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0;
    printf("수행시간: %Lfs.\n", time);
    

    /*sleep(4);
    while(1){
        cout << "Success Pagerank execution" << endl;
        sleep(1200); 
    }*/
    
}