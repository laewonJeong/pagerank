#include "pagerank.hpp"
#include "../../includes/network/myRDMA.hpp"

myRDMA myrdma1;
Pagerank pagerank;
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
    pagerank.new_pr[i] = (1-df)/pagerank.num_of_vertex + tmp;
        //cout << "pr[" <<i<<"]: " << pagerank.new_pr[i] <<endl;*/
    //double my_pr = df*(pagerank.pr[i]/pagerank.graph[i].size());
}
void Pagerank::calc_pagerank_value(){
    vector<thread> worker;
    int check = 0;
    for(int i=0;i<pagerank.num_of_vertex;i++){
        worker.push_back(thread(&Pagerank::thread_calc_pr,i));
    }
    for(int i=0;i<pagerank.num_of_vertex;i++){
        cout << "pr[" <<i<<"]: " << pagerank.new_pr[i] <<endl;
        worker[i].join();
    }
    cout << endl;
}

void Pagerank::change_pagerank_value(){
    for(int i = 0;i<pagerank.num_of_vertex;i++){
        pagerank.pr[i] = pagerank.new_pr[i];
    }
}

void Pagerank::run_pagerank(int iter){
    int step;
    for(int step =0; step < iter ;step++){
        cout <<"====="<< step+1 << " step=====" <<endl;
        Pagerank::calc_pagerank_value();
        myrdma1.rdma_comm("send", to_string(pagerank.new_pr[0]));
        cout << pagerank.recv_buffer[0] << endl;
        if(pagerank.pr==pagerank.new_pr)
            break;
        Pagerank::change_pagerank_value();
    }
    cout << "Done." << endl;
}

int Pagerank::max_pr(){
    int important = 0;
    double important_pr = pagerank.pr[0];
    double tmp = important_pr;
    for (int i=0;i< pagerank.num_of_vertex;i++){
        important_pr = max(important_pr, pagerank.pr[i]);
        if(tmp != important_pr){
            important = i;
            tmp = important_pr;
        }
    }
    return important;
}

void Pagerank::init_connection(const char* ip, string server[], int number_of_server, int Port)
{
    myrdma1.initialize_rdma_connection(ip,server,number_of_server,Port,pagerank.send_buffer,pagerank.recv_buffer);
    myrdma1.create_rdma_info();
    myrdma1.send_info_change_qp();
}