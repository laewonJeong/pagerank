#include "pagerank.hpp"
#include "../../includes/network/myRDMA.hpp"
#include "../../includes/network/tcp.hpp"
#include <boost/lexical_cast.hpp>
#include <numeric>
#include <time.h>

TCP tcp1;
myRDMA myrdma1;
Pagerank pagerank;
vector<int> sock_idx;
static std::mutex mutx;
vector<double> send_buffer[4];
vector<double> recv_buffer[4];
int n, n1;

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
        
        pagerank.graph.resize(max_dim);;
        if (pagerank.num_outgoing.size() <= max_dim) {
            pagerank.num_outgoing.resize(max_dim,0);
        }
    }

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
    

    if(pagerank.my_ip == pagerank.node[pagerank.num_of_server-1])
        send_buffer[0].resize(n1);
    else{
        send_buffer[0].resize(n);
    }
    

    cout << "Done" <<endl;
}

void Pagerank::calc_pagerank_value(int start, int end, double x, double y){
    const int num_of_vertex = pagerank.num_of_vertex;
    double df_inv = 1.0 - df;
    double inv_num_of_vertex = 1.0 / num_of_vertex;
    const vector<vector<size_t>>& graph = pagerank.graph;
    const vector<int>& num_outgoing = pagerank.num_outgoing;
    
    double* recv_buffer_ptr = recv_buffer[0].data();    
    double* send_buffer_ptr = send_buffer[0].data();

    for(size_t i=start;i<end;i++){
        double tmp = 0.0;
        const size_t graph_size = graph[i].size();
        const size_t* graph_ptr = graph[i].data();

        for(size_t j=0; j<graph_size; j++){
            const size_t from_page = graph_ptr[j];
            const double inv_num_outgoing = 1.0 / num_outgoing[from_page];

            tmp += recv_buffer_ptr[from_page] * inv_num_outgoing;
        }
        send_buffer_ptr[i-start] = (tmp + x * inv_num_of_vertex) * df + df_inv * inv_num_of_vertex;
    }
}


void Pagerank::run_pagerank(int iter){
    cout << "progressing..." << endl;
    
    vector<double> prev_pr;
    size_t step;
    pagerank.diff = 1;
    string my_ip = pagerank.my_ip;
    string server_ip = pagerank.server_ip;
    int start = pagerank.start1;
    int end = pagerank.end1;
    int num_of_vertex = pagerank.num_of_vertex;
    double diff;
    double dangling_pr = 0.0;
    const vector<int>& num_outgoing = pagerank.num_outgoing;
    for(step =0; step < iter ;step++){
        
        cout <<"====="<< step+1 << " step=====" <<endl;
        
        dangling_pr = 0.0;
        if(step!=0) {
            if(my_ip != server_ip){
                for (size_t i=0;i<num_of_vertex;i++) {
                    if (num_outgoing[i] == 0)
                        dangling_pr += recv_buffer[0][i];   
                }
            }
            else{
                diff = 0;
                for (size_t i=0;i<num_of_vertex;i++) 
                    diff += fabs(prev_pr[i] - send_buffer[0][i]);
                pagerank.diff = diff;
            }
            
        }

        if(my_ip != server_ip)
            Pagerank::calc_pagerank_value(start,end,dangling_pr,0.0);
        else
            prev_pr = send_buffer[0];
        
        Pagerank::gather_pagerank("send");
        
        Pagerank::scatter_pagerank();
        
        if(my_ip == server_ip)
            cout << pagerank.diff << endl;

        if(pagerank.diff < 0.00001 || recv_buffer[0][0] > 1){
            break;
        }

    }
    
}

string Pagerank::max_pr(){
    int important = 0;
    double important_pr = recv_buffer[0][0]-1;

    for (int i = 1; i < pagerank.num_of_vertex; ++i) {
        if (recv_buffer[0][i] > important_pr) {
            important = i;
            important_pr = recv_buffer[0][i];
        }
    }

    stringstream ss;
    ss << "important page is " << important << " and value is " << important_pr;
    return ss.str();
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
    pagerank.node = server;
    pagerank.server_ip = server[0];

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
void fill_send_buffer(int num_of_server, int index){
    int size = n;
    
    for(int i=0;i<num_of_server-1;i++){
        if(i == index)
            size = n1;
        send_buffer[0].insert(send_buffer[0].end(),recv_buffer[i].begin(),recv_buffer[i].begin()+size);
    }   
 
}
void Pagerank::gather_pagerank(string opcode){
    if(pagerank.my_ip == pagerank.server_ip){
        myrdma1.rdma_many_to_one_recv_msg(opcode);
        
        send_buffer[0].clear();

        fill_send_buffer(pagerank.num_of_server, pagerank.num_of_server-2);

        if(pagerank.diff < 0.00001)
            send_buffer[0][0] += 1; 
            
        fill(&send_buffer[1], &send_buffer[pagerank.num_of_server-1], send_buffer[0]);    
    }
    else{
        myrdma1.rdma_many_to_one_send_msg(opcode,"s",send_buffer[0]);
    }

}

void Pagerank::scatter_pagerank(){
    if(pagerank.my_ip == pagerank.server_ip){
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
    double sum1 = accumulate(recv_buffer[0].begin(), recv_buffer[0].end(), -1.0);
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