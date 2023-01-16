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
    time_t begin, end;
    begin = time(NULL);

    // Create graph data
    pagerank.create_graph_data(data_path, graph_data_vertex);

    // Initial pr value each vertex
    pagerank.initial_pagerank_value();

    //calc pagerank
    pagerank.run_pagerank(iter);

    // important thing
    int important = pagerank.max_pr();
    cout << "important page is " << important<< " page." <<endl;

    // check time
    end = time(NULL);
    //long time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec);
    printf("수행시간: %lfs\n", (double)(end-begin));

    /*sleep(4);
    while(1){
        cout << "Success Pagerank execution" << endl;
        sleep(1200); 
    }*/
    
}