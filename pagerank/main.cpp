#include<time.h>
#include "pagerank.hpp"
#include "myRDMA.hpp"

#define port 40145
#define num_of_node 4
#define server_ip "192.168.1.100"
#define iter 100000

string node[num_of_node] = {server_ip,"192.168.1.101","192.168.1.102","192.168.1.103"};
char send_buffer[num_of_node][buf_size];
char recv_buffer[num_of_node][buf_size];

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
    Pagerank pagerank;
    int start1, end1;
    if(node[0] == argv[1]){
        start1 = 0;
        end1 = 4039/num_of_node;
        cout << start1 << " " << end1 <<endl;
    }
    else if(node[1] == argv[1]){
        start1 = 4039/num_of_node;
        end1 = start1 + 4039/num_of_node;
        cout << start1 << " " << end1 <<endl;
    }
    else if(node[2] == argv[1]){
        start1 = 4039/num_of_node + 4039/num_of_node;
        end1 = start1 + 4039/num_of_node;
        cout << start1 << " " << end1 <<endl;
    }
    else if(node[3] == argv[1]){
        start1 = 4039/num_of_node + 4039/num_of_node+ 4039/num_of_node;
        end1 = start1 + 4039/num_of_node;
        cout << start1 << " " << end1 <<endl;
    }
    pagerank.init_connection(argv[1], node, num_of_node, port);    

    cout <<"--------------------------------------------------" << endl;
    time_t start, end;
    start = time(NULL);

    // Create graph data
    string data_path = "/home/lwjeong/git/pagerank/pagerank/facebook_data.txt";
    pagerank.create_graph_data(data_path, 4039);

    // Initial pr value each vertex
    pagerank.initial_pagerank_value();

    //calc pagerank
    pagerank.run_pagerank(iter,start1,end1);

    // important thing
    int important = pagerank.max_pr();
    cout << "important page is " << important<< " page." <<endl;

    // check time
    end = time(NULL);
    cout << "수행시간: " <<(double)(end-start) <<"s" <<endl;

    /*sleep(4);
    while(1){
        cout << "Success Pagerank execution" << endl;
        sleep(1200); 
    }*/
    
}