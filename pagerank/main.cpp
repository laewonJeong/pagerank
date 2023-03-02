#include<time.h>
#include "pagerank.hpp"
#include "myRDMA.hpp"
#include "tcp.hpp"
#include <map>

#define num_of_node 5
#define port 40145
#define server_ip "192.168.1.100"
#define iter 100000
string node1[num_of_node] = {server_ip,"192.168.1.101","192.168.1.102","192.168.1.103","192.168.1.104"};//,"192.168.1.102","192.168.1.103"};
using namespace std;
//using namespace stdext;

bool is_server(string ip){
  if(ip == server_ip)
    return true;
  return false;
}

int main(int argc, char* argv[]){
    vector<long double> x1[num_of_node];
    vector<long double> recv[num_of_node];
    vector<long double> x;
    Pagerank pagerank;
    TCP tcp;
    myRDMA myrdma;
    string data_path = argv[1];
 
    string my_ip = tcp.check_my_ip();
    cout<<"my ip is " << my_ip << endl;

    pagerank.create_graph_data(data_path);
    int num_of_vertex = pagerank.get_num_of_vertex();


    myrdma.initialize_rdma_connection_vector(my_ip.c_str(),node1,num_of_node,port,x1,recv,num_of_vertex);
    myrdma.create_rdma_info();
    myrdma.send_info_change_qp();
    cout << "--------------------------------------------------------" << endl;
    for(int i=0;i<20;i++){
        x.push_back(i);
    }

    if(is_server(my_ip)){
        myrdma.rdma_many_to_one_recv_msg("send");
    }
    else{
        myrdma.rdma_many_to_one_send_msg("send","s",x);
    }
    
    if(is_server(my_ip)){
        for(int i=0;i<num_of_node-1;i++)
            myrdma.rdma_send_pagerank(x,i);
    }
    else{
        myrdma.rdma_recv_pagerank(0);
        for(int j =0 ;j<80;j++){
            cout << recv[0][j] << " ";
        }
    }


    /*int partition;
    for(int i=0; i<num_of_node; i++){
        if(my_ip == node1[num_of_node-1]){
            int n1 = pagerank.get_num_of_vertex() - n*(num_of_node-1) + n;
            partition=n1;
        }
        else{
            partition=n;
        }
    }

    cout << partition << endl;*/
    
    /*if(argc != 3){
        cerr << argv[0] << " <MY IP> " << endl;
        exit(1);
    }*/
    /*if(server_ip != node1[0]){
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

    map<int, vector<int>> m;
    m[0].push_back(2);
    m[0].push_back(3);
    cout << m[0][0] << " " << m[0][1] << endl;

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
    //pagerank.print_pr();

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