#include "pagerank.hpp"
#include "../../includes/network/myRDMA.hpp"

myRDMA myrdma1;
Pagerank pagerank;
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
            vector<string> split;
            istringstream ss(line);
            string stringBuffer;
            while(getline(ss, stringBuffer, ' ')){
                split.push_back(stringBuffer);
            }
            pagerank.graph[stoi(split[0])].push_back(stoi(split[1]));
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
        pagerank.my_pr.push_back(0.0);
    }
    cout << "Done" <<endl;
}

void Pagerank::thread_calc_pr(int i){
    double tmp = 0;
    for(int j=0;j<pagerank.num_of_vertex;j++){
        if(i == j)
            continue;
        if(find(pagerank.graph[j].begin(), pagerank.graph[j].end(), i) != pagerank.graph[j].end())
                tmp += df*(pagerank.pr[j]/pagerank.graph[j].size());
    }
    pagerank.new_pr[i] = stod(to_string((1-df)/pagerank.num_of_vertex + tmp));
    //cout << "pr[" <<i<<"]: " << pagerank.new_pr[i] <<endl;
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


    cout << endl;
}

void Pagerank::change_pagerank_value(){
    //for(int i = 0;i<pagerank.num_of_vertex;i++){
    pagerank.pr = pagerank.new_pr;
        //cout << "pr[" <<i<<"]: " << pagerank.pr[i] <<endl;
    //}
}
void Pagerank::combine_pr(){
    for(int i=0;i<5;i++){
        vector<string> a;
        string tmp(pagerank.recv_buffer[i]);
        a = split(tmp,' ');
        for(int j=0;j<a.size();j++){
            if(j%2!=0){
                pagerank.new_pr[stoi(a[j])] = stod(a[j+1]);
            }
        }
    }
    for(int i = 0;i<pagerank.num_of_vertex;i++){
        cout << "pr[" <<i<<"]: " << pagerank.pr[i] <<endl;
    }
    
}
void Pagerank::send_recv_pagerank_value(int start, int end){
    string message = "";
    for(int i=start;i<end;i++){
        message = message + to_string(i);
        message = message + " " + to_string(pagerank.new_pr[i]) + " ";
    }
    myrdma1.rdma_comm("send", message);
}
void Pagerank::run_pagerank(int iter){
    int step;
    for(int step =0; step < iter ;step++){
        cout <<"====="<< step+1 << " step=====" <<endl;
        Pagerank::calc_pagerank_value(pagerank.start1,pagerank.end1);
        Pagerank::send_recv_pagerank_value(pagerank.start1,pagerank.end1);
        Pagerank::combine_pr();
        if(pagerank.pr==pagerank.new_pr){
            break;
        }
        //Pagerank::change_pagerank_value();
        pagerank.pr = pagerank.new_pr;
    }
    cout << "Done." << endl;
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