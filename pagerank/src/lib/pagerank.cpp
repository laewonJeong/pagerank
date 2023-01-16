#include "pagerank.hpp"
#include "../../includes/network/myRDMA.hpp"
#include "../../includes/network/tcp.hpp"

TCP tcp1;
myRDMA myrdma1;
Pagerank pagerank;
vector<int> sock_idx;
vector<string> split(string str, char Delimiter) {
    istringstream iss(str);             
    string buffer;                     
    vector<string> result;
 
    while (getline(iss, buffer, Delimiter)) {
        result.push_back(buffer);   
    }
    return result;
}
void Pagerank::create_graph_data(string path, int num_of_vertex){
    cout << "creating graph data" <<endl;
    pagerank.num_of_vertex = num_of_vertex;

    string line;
	ifstream file(path);
	if(file.is_open()){
        while(getline(file, line)) {
            vector<string> s;
            s = split(line, ' ');
            pagerank.graph[stoi(s[0])].push_back(stoi(s[1]));
            pagerank.outgoing[stoi(s[1])].push_back(stoi(s[0]));
		}
		file.close();
        cout << "Done" <<endl;
	} 
    else {
		cout << "Unable to open file" <<endl;
        exit(1);
	}
}

void Pagerank::initial_pagerank_value(){
    cout << "init pagerank value" << endl;
    for(int i=0;i<pagerank.num_of_vertex;i++){
        pagerank.pr.push_back(1.0/pagerank.num_of_vertex);
        pagerank.new_pr.push_back(0);
    }
    cout << "Done" <<endl;
}

void Pagerank::thread_calc_pr(int i){
    double tmp = 0;
    for(int j = 0; j<pagerank.outgoing[i].size();j++){
        tmp+=df*(pagerank.pr[pagerank.outgoing[i][j]]/pagerank.graph[pagerank.outgoing[i][j]].size());
    }
    pagerank.new_pr[i] = stod(to_string((1-df)/pagerank.num_of_vertex + tmp));
}

void Pagerank::calc_pagerank_value(int start, int end){
    vector<thread> worker;
    int check = 0;
    for(int i=start;i<end;i++){
        worker.push_back(thread(&Pagerank::thread_calc_pr,i));
    }
    for(int i=0;i<end-start;i++){
        worker[i].join();
    }


   //cout << endl;
}

void Pagerank::combine_pr(){
    //cout << "start combine" << endl;
    /*for (int i=0;i<5;i++){
        char tcp_recv_buffer[buf_size1];
        //cout << "start combine" << endl;
        string s;
        s = tcp1.recv_message(sock_idx[i]);
        //cout << "start combine" << endl;
        strcpy(tcp_recv_buffer, s.c_str());
        //cout << "start combine" << endl;
        vector<string> a;
        //cout << "start combine" << endl;
        string tmp(tcp_recv_buffer);
        //cout << "start combine" << endl;
        a = split(tmp,' ');
        //cout << "start combine" << endl;
        for(int j=0;j<a.size();j++){
            if(j%2==0){
                cout << stoi(a[j]) << " " << stod(a[j+1]) << endl;
                pagerank.new_pr[stoi(a[j])] = stod(a[j+1]);
            }
        }
        //cout << "start combine" << endl;
    }
    //cout << "start combine" << endl;*/

    for(int i=0;i<3;i++){
        vector<string> a;
        string tmp(pagerank.recv_buffer[i]);
        a = split(tmp,' ');
        for(int j=0;j<a.size();j++){
            if(j%2==0){
                pagerank.new_pr[stoi(a[j])] = stod(a[j+1]);
            }
        }
    }
    /*for(int i = 0;i<pagerank.num_of_vertex;i++){
        cout << "pr[" <<i<<"]: " << pagerank.pr[i] <<endl;
    }*/
    
}
void Pagerank::send_recv_pagerank_value(int start, int end){
    //cout << "sending" << endl;
    string message = "";
    for(int i=start;i<end;i++){
        message = message + to_string(i)+" " + to_string(pagerank.new_pr[i]) + " ";
        //cout << "pr[" <<i<<"]: " << pagerank.pr[i] <<endl;
    }
   /* for(int i = 0;i<5;i++){
        tcp1.send_msg(message.c_str(),sock_idx[i]);
    }*/
    myrdma1.rdma_comm("write", message);
    //cout << "finish sending" << endl;
}
void Pagerank::run_pagerank(int iter){
    int step;
    /*int *clnt_socks = tcp1.client_sock();
    for(int idx=0; idx < 6; idx++){
        if(clnt_socks[idx]!=0){
            sock_idx.push_back(idx);
        }
    }*/
    cout << "progressing..." << endl;
    for(int step =0; step < iter ;step++){
        //cout <<"====="<< step+1 << " step=====" <<endl;
        Pagerank::calc_pagerank_value(pagerank.start1,pagerank.end1);
        Pagerank::send_recv_pagerank_value(pagerank.start1,pagerank.end1);
        Pagerank::combine_pr();
        if(pagerank.pr==pagerank.new_pr){
            break;
        }
        //Pagerank::change_pagerank_value();
        pagerank.pr = pagerank.new_pr;
    }
    cout << "Done" << endl;
}

int Pagerank::max_pr(){
    int important = 0;
    long double important_pr = pagerank.pr[0];
    long double tmp = important_pr;
    for (int i=0;i< pagerank.num_of_vertex;i++){
        important_pr = max(important_pr, pagerank.pr[i]);
        if(tmp != important_pr){
            important = i;
            tmp = important_pr;
        }
    }
    return important;
}

void Pagerank::init_connection(const char* ip, string server[], int number_of_server, int Port, int number_of_vertex)
{
    myrdma1.initialize_rdma_connection(ip,server,number_of_server,Port,pagerank.send_buffer,pagerank.recv_buffer);
    myrdma1.create_rdma_info();
    myrdma1.send_info_change_qp();
    for(int i=0;i<number_of_server;i++){
        if(ip == server[i]){
            pagerank.start1 = number_of_vertex/number_of_server*i;
            pagerank.end1 = pagerank.start1 + number_of_vertex/number_of_server;
        }
        if(ip == server[number_of_server-1]){
            pagerank.end1 = number_of_vertex;
        }
    }
    cout << pagerank.start1 << " " <<pagerank.end1 <<endl;
}