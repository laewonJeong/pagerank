#include<time.h>
#include "pagerank.hpp"
#include "myRDMA.hpp"

#define port 40145
#define server_ip "192.168.1.100"
#define iter 100000
string node1[4] = {server_ip,"192.168.1.101","192.168.1.102","192.168.1.103"};
using namespace std;
int main(int argc, char* argv[]){
    /*if(argc != 3){
        cerr << argv[0] << " <MY IP> " << endl;
        exit(1);
    }*/
    if(server_ip != node1[0]){
        cerr << "node[0] is not server_ip" << endl;
        exit(1);
    }
    int num_of_node = atoi(argv[3]);
    if(num_of_node != 1 && num_of_node !=4){
        cerr <<"1 or 4" << endl;
        exit(1);
    }
    string node[num_of_node];
    if(num_of_node == 1){
        node[0] = argv[1];
    }
    else{
        for(int i=0;i<num_of_node;i++){
            node[i] = node1[i];
        }
    }

    Pagerank pagerank;

    string data_path = argv[2];
    pagerank.create_graph_data(data_path);
    //int graph_data_vertex = 4039;
    
    // init connection for rdma communication
    pagerank.init_connection(argv[1], node, num_of_node, port);    

    cout << "-------------------------------------" <<endl;
    // check time
    struct timespec begin, end ;
    clock_gettime(CLOCK_MONOTONIC, &begin);

    // Create graph data
    

    // Initial pr value each vertex
    pagerank.initial_pagerank_value();

    //calc pagerank
    pagerank.run_pagerank(iter);

    // check time
    clock_gettime(CLOCK_MONOTONIC, &end);
    long double time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0;


    //print pagerank value
    pagerank.print_pr();

    cout << "Done." << endl;
    cout << "-------------------------------------" <<endl;
    printf("수행시간: %Lfs.\n", time);

    // important thing
    cout << "-------------------------------------" <<endl;
    string important = pagerank.max_pr();
    cout << "-------------------------------------" <<endl;

   
     /*sleep(4);
    while(1){
        cout << "Success Pagerank execution" << endl;
        sleep(1200); 
    }*/
    
}