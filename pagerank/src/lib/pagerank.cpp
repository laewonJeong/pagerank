#include "pagerank.hpp"
#include "../../includes/network/myRDMA.hpp"
#include "../../includes/network/tcp.hpp"
#include <boost/lexical_cast.hpp>
#include <numeric>
#include <time.h>
//#include <mpi.hpp>
//#include <strtk.hpp>

TCP tcp1;
myRDMA myrdma1;
Pagerank pagerank;
vector<int> sock_idx;
vector<long double> old_pr;
static std::mutex mutx;
vector<long double> send_buffer[5];
vector<long double> recv_buffer[5];
vector<long double> real_pr;
int n, n1;
//string message = "";

vector<string> split(string str, char Delimiter) {
    istringstream iss(str);             
    string buffer;                     
    vector<string> result;
 
    while (getline(iss, buffer, Delimiter)) {
        result.push_back(buffer);   
    }
    return result;
}
template <class Vector, class T>
bool Pagerank::insert_into_vector(Vector& v, const T& t) {
    typename Vector::iterator i = lower_bound(v.begin(), v.end(), t);
    if (i == v.end() || t < *i) {
        v.insert(i, t);
        return true;
    } else {
        return false;
    }
}
bool Pagerank::add_arc(size_t from, size_t to) {
    vector<size_t> v;
    bool ret = false;
    size_t max_dim = max(from, to);

    if (pagerank.graph.size() <= max_dim) {
        max_dim = max_dim + 1;
        
        pagerank.graph.resize(max_dim);
        //pagerank.outgoing.resize(max_dim);
        if (pagerank.num_outgoing.size() <= max_dim) {
            pagerank.num_outgoing.resize(max_dim,0);
        }
    }
    //pagerank.graph[to].push_back(from);
    //cout << pagerank.graph[to] << endl;

    ret = insert_into_vector(pagerank.graph[to], from);

    if (ret) {
        pagerank.num_outgoing[from]++;
    }

    return ret;
}
void Pagerank::create_graph_data(string path, string del){
    cout << "Creating graph about  "<< path<<"..."  <<endl;
    pagerank.num_of_vertex = num_of_vertex;
    istream *infile;

    infile = new ifstream(path.c_str());
    size_t line_num = 0;
    string line;
	
	if(infile){
        while(getline(*infile, line)) {
            string from, to;
            size_t pos;
            if(del == " ")
                pos = line.find(" ");
            else{
                pos = line.find("\t");
            }
            from = line.substr(0,pos);
            to = line.substr(pos+1);
            /*if(pagerank.pr1.find(from) == pagerank.pr1.end()){
                pagerank.pr1[from] = 0.0;
                //cout << from << ": " <<pagerank.pr1[from] << endl;
            }*/
            add_arc(strtol(from.c_str(), NULL, 10),strtol(to.c_str(), NULL, 10));
            line_num++;
            if(line_num%500000 == 0)
                cerr << "Create " << line_num << " lines" << endl;
		}
	} 
    else {
		cout << "Unable to open file" <<endl;
        exit(1);
	}
    //agerank.new_pr1 = pagerank.pr1;
    //pagerank.pr1["0"] = 1;
    pagerank.num_of_vertex = pagerank.graph.size();
    cerr << "Create " << line_num << " lines, "
         << pagerank.num_of_vertex << " vertices graph." << endl;
    
    cerr << "----------------------------------" <<endl;
    delete infile;
}

void Pagerank::initial_pagerank_value(){
    cout << "init pagerank value" << endl;
   
    n = pagerank.num_of_vertex/(pagerank.num_of_server-1);
    
    n1 = pagerank.num_of_vertex - n*(pagerank.num_of_server-2);
    

    //pagerank.pr.resize(pagerank.num_of_vertex, 1/pagerank.num_of_vertex);
    //pagerank.pr1.reserve(pagerank.num_of_vertex);
    //pagerank.new_pr1.resize(pagerank.num_of_vertex,"0");
    //pagerank.new_pr1[0] = "1";
    if(pagerank.my_ip == "192.168.0.106")
        send_buffer[0].resize(n1);
    else{
        send_buffer[0].resize(n);
    }
    
    //pagerank.pr[0] = 1;

    cout << "Done" <<endl;
}

void Pagerank::thread_calc_pr(int i, double x, double y){
    double tmp = 0;
    
    for(int j = 0; j<pagerank.graph[i].size();j++){
        tmp += df*(recv_buffer[0][pagerank.graph[i][j]]/pagerank.num_outgoing[pagerank.graph[i][j]]);
    }
    pagerank.new_pr[i] = stod(to_string((1-df)/pagerank.num_of_vertex + tmp));
    pagerank.diff += fabs(pagerank.new_pr[i] - recv_buffer[0][i]);
    //cout << pagerank.new_pr[i] <<endl;    
}

void Pagerank::calc_pagerank_value(int start, int end, double x, double y){
    
    for(int i=start;i<end;i++){
        double tmp = 0.0;

        for(int j=0; j<pagerank.graph[i].size(); j++){
            int from_page = pagerank.graph[i][j];
            tmp += recv_buffer[0][from_page]/pagerank.num_outgoing[from_page];
        }
        
        send_buffer[0][i-start] = (tmp + x/pagerank.num_of_vertex)*df + (1-df)/pagerank.num_of_vertex;
       
    }
}

void Pagerank::combine_pr(){
    
}
void Pagerank::send_recv_pagerank_value(int start, int end){
    
    myrdma1.rdma_comm("write", pagerank.message);
}
void Pagerank::run_pagerank(int iter){
    double prev_diff =0;
    vector<long double> prev_pr;
    long double prev_sum;
    long double cur_sum;
    long double time;
    int step;
    size_t i;
    double sum_pr; // sum of current pagerank vector elements
    double dangling_pr; // sum of current pagerank vector elements for dangling
    struct timespec begin, end ;
    //size_t num_rows = pagerank.graph.size();
    pagerank.diff = 1;
    cout << "progressing..." << endl;
    //&send_buffer[1] = &send_buffer[0].data();
    for(int step =0; step < iter ;step++){
        cout <<"====="<< step+1 << " step=====" <<endl;
        
        if(step!=0) {
            dangling_pr = 0;
            pagerank.diff = 0;
            for (i=0;i<pagerank.num_of_vertex;i++) {
                if (pagerank.num_outgoing[i] == 0)
                    dangling_pr += recv_buffer[0][i];   
            }
        }

        if(pagerank.my_ip != "192.168.0.100"){
            clock_gettime(CLOCK_MONOTONIC, &begin);

            Pagerank::calc_pagerank_value(pagerank.start1,pagerank.end1,dangling_pr,0.0);

            clock_gettime(CLOCK_MONOTONIC, &end);
            time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0;
            printf("PageRank 계산 시간: %Lfs.\n", time);
        }
        
       
        
        clock_gettime(CLOCK_MONOTONIC, &begin);
        
        Pagerank::gather_pagerank("send",0,pagerank.new_pr);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0;
        printf("gather 수행시간: %Lfs.\n", time);
        
        
        //pagerank.pr = pagerank.new_pr;
        //prev_pr = recv_buffer[0];
        
        clock_gettime(CLOCK_MONOTONIC, &begin);
        
        Pagerank::scatter_pagerank("send",0,pagerank.new_pr);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0;
        printf("scatter 수행시간: %Lfs.\n", time);
        
        //if(pagerank.my_ip != "192.168.0.100")
            //pagerank.pr = recv_buffer[0];
        
        //else{
        //cout.precision(numeric_limits<double>::digits10);
        //cout << pagerank.diff <<endl;  //<< " " << prev_diff << " = " << z <<endl;
        //}

        if(step == 61){//pagerank.diff < 0.00001){//pagerank.diff < 0.00001){//fabs(pagerank.diff - prev_diff) <0.0000001){
            break;
        }
        //prev_diff = pagerank.diff;
        //pagerank.pr = pagerank.new_pr;

    }
    
}

string Pagerank::max_pr(){
    int important = 0;
    string result = "";
    long double important_pr = recv_buffer[0][0];
    long double tmp = important_pr;
    //double sum1 = accumulate(pagerank.pr.begin(), pagerank.pr.end(), 0.0);
    for (int i=0;i< pagerank.num_of_vertex;i++){
        important_pr = max(important_pr, recv_buffer[0][i]);
        if(tmp != important_pr){
            important = i;
            tmp = important_pr;
        }
    }

    cout << "important page is " << important << " and value is " << tmp << endl;

    result += to_string(important);
    return result;
}

void Pagerank::init_connection(const char* ip, string server[], int number_of_server, int Port, int num_of_vertex)
{
    myrdma1.initialize_rdma_connection_vector(ip,server,number_of_server,Port,send_buffer,recv_buffer,num_of_vertex);
    myrdma1.create_rdma_info();
    myrdma1.send_info_change_qp();
    string str_ip(ip);
    pagerank.my_ip = str_ip; 
    pagerank.num_of_server = number_of_server;
    pagerank.diff = 1;
    for(int i=1;i<number_of_server;i++){
        if(ip == server[i]){
            pagerank.start1 = pagerank.num_of_vertex/(number_of_server-1)*(i-1);
            pagerank.end1 = pagerank.start1 + pagerank.num_of_vertex/(number_of_server-1);
        }
        if(ip == server[number_of_server-1]){
            pagerank.end1 = pagerank.num_of_vertex;
        }
    }
    cout << pagerank.start1 << " " <<pagerank.end1 <<endl;
}
void Pagerank::gather_pagerank(string opcode, int i, vector<long double> pr){
    int j;
    if(pagerank.my_ip == "192.168.0.100"){
        myrdma1.rdma_many_to_one_recv_msg("send");
        send_buffer[0].clear();
        //send_buffer[0].reserve(pagerank.num_of_vertex);
        for(int i=0;i<pagerank.num_of_server-1;i++){
            if(i == pagerank.num_of_server-2)
                send_buffer[0].insert(send_buffer[0].end(),recv_buffer[i].begin(),recv_buffer[i].begin()+n1);
            else
                send_buffer[0].insert(send_buffer[0].end(),recv_buffer[i].begin(),recv_buffer[i].begin()+n);
        }
    }
    else{
        myrdma1.rdma_many_to_one_send_msg("send","s",pr);
    }

}
void Pagerank::scatter_pagerank(string opcode, int i, vector<long double> pr){
    if(pagerank.my_ip == "192.168.0.100"){
        for(int i=0;i<pagerank.num_of_server-1;i++)
            myrdma1.rdma_send_pagerank(send_buffer[0],i);
    }
    else{
        myrdma1.rdma_recv_pagerank(0);
    }
}


void Pagerank::print_pr(){
    size_t i;
    double sum = 0;
    double sum1 = accumulate(recv_buffer[0].begin(), recv_buffer[0].end(), 0.0);
    cout.precision(numeric_limits<double>::digits10);
    for(i=pagerank.num_of_vertex-200;i<pagerank.num_of_vertex;i++){
        cout << "pr[" <<i<<"]: " << recv_buffer[0][i] <<endl;
        sum += recv_buffer[0][i];
    }
    cerr << "s = " <<round(sum1) << endl;
}

int Pagerank::get_num_of_vertex(){
    return pagerank.num_of_vertex;
}