#include "pagerank.hpp"
#include "../../includes/network/myRDMA.hpp"
#include "../../includes/network/tcp.hpp"
#include <boost/lexical_cast.hpp>
#include <numeric>
//#include <strtk.hpp>

TCP tcp1;
myRDMA myrdma1;
Pagerank pagerank;
vector<int> sock_idx;
vector<long double> old_pr;
static std::mutex mutx;
char send_buffer[4][buf_size1];
char recv_buffer[4][buf_size1];
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

    ret = insert_into_vector(pagerank.graph[to], from);
    if (ret) {
        pagerank.num_outgoing[from]++;
    }

    return ret;
}
void Pagerank::create_graph_data(string path){
    cout << "Creating graph about  "<< path<<"..."  <<endl;
    pagerank.num_of_vertex = num_of_vertex;
    istream *infile;

    infile = new ifstream(path.c_str());
    size_t line_num = 0;
    string line;
	
	if(infile){
        while(getline(*infile, line)) {
            string from, to;
            size_t pos = line.find(" ");
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

    pagerank.pr.resize(pagerank.num_of_vertex, 1/pagerank.num_of_vertex);
    //pagerank.new_pr1.resize(pagerank.num_of_vertex,"0");
    //pagerank.new_pr1[0] = "1";
    pagerank.new_pr = pagerank.pr;
    //pagerank.pr[0] = 1;

    cout << "Done" <<endl;
}

void Pagerank::thread_calc_pr(int i, double x, double y){
    double tmp = 0;
    
    for(int j = 0; j<pagerank.graph[i].size();j++){
        tmp += df*(pagerank.pr[pagerank.graph[i][j]]/pagerank.num_outgoing[pagerank.graph[i][j]]);
    }
    pagerank.new_pr[i] = stod(to_string((1-df)/pagerank.num_of_vertex + tmp));
    pagerank.diff += fabs(pagerank.new_pr[i] - pagerank.pr[i]);
    //cout << pagerank.new_pr[i] <<endl;    
}

void Pagerank::calc_pagerank_value(int start, int end, double x, double y){
    pagerank.diff = 0;
    long double tmp;
    double d;
    string value;
    //double sum = 0;
    for(int i=start;i<end;i++){
        tmp = 0;
        for(int from_page: pagerank.graph[i]){
            tmp += pagerank.pr[from_page]/pagerank.num_outgoing[from_page];
            //tmp += pagerank.pr[pagerank.graph[i][j]]/pagerank.num_outgoing[pagerank.graph[i][j]];
        }
        
        if(pagerank.num_of_server != 1){
            d = (tmp + x/pagerank.num_of_vertex)*df + (1-df)/pagerank.num_of_vertex;
            value = to_string(d*100000000000);

            pagerank.new_pr[i] = d;
            
            pagerank.message += to_string(i);
            pagerank.message += " ";
            pagerank.message += value; 
            pagerank.message += "\n";
        }
        else{
            pagerank.new_pr[i] = (tmp + x/pagerank.num_of_vertex)*df + (1-df)/pagerank.num_of_vertex;
        }

        pagerank.diff += fabs(pagerank.new_pr[i] - pagerank.pr[i]);
    }
    //cout << "s = " << sum <<endl;
    
}
void Pagerank::thread_combine_pr(int i){
    string from, to;
    size_t previous, current;

    double d;
    int f;
    string a;
    
    //strcpy(tmp, pagerank.recv_buffer[i]);
    string tmp(recv_buffer[i]);
    current = tmp.find('\n');
    previous = 0;
    while(current != string::npos){
        a = tmp.substr(previous, current - previous);
        size_t pos = a.find(" ");
        from = a.substr(0,pos);
        to = a.substr(pos+1);
        f = stoi(from);
        
        d = stod(to);
        //cout <<from << ": " <<to << endl;
        pagerank.new_pr[f] = d/100000000000;//stod(to);
        //mutx.lock();
        pagerank.diff += fabs(pagerank.new_pr[f] - pagerank.pr[f]);
        //mutx.unlock();  
        //diff += fabs(pagerank.pr[stoi(from)] - old_pr[stoi(from)]);
        previous = current +1;
        current = tmp.find('\n',previous);
    }
}
void Pagerank::combine_pr(){
    //230126 thread를 시도해보자
    //vector<thread> worker;
    for(int i = 0; i < 3;i++){
        Pagerank::thread_combine_pr(i);
        //worker.push_back(thread(&Pagerank::thread_combine_pr,i));
    }
    /*for(int i=0;i<3;i++){
        worker[i].detach();
    }*/
}
void Pagerank::send_recv_pagerank_value(int start, int end){
    
    myrdma1.rdma_comm("write", pagerank.message);
}
void Pagerank::run_pagerank(int iter){
    double prev_diff =0;
    int step;
    size_t i;
    double sum_pr; // sum of current pagerank vector elements
    double dangling_pr; // sum of current pagerank vector elements for dangling
    //size_t num_rows = pagerank.graph.size();

    cout << "progressing..." << endl;
    for(int step =0; step < iter ;step++){
        cout <<"====="<< step+1 << " step=====" <<endl;
        pagerank.message = "";
        sum_pr = 0;
        dangling_pr = 0;

        if(step!=0) {
            // Normalize so that we start with sum equal to one   
            //double sum1 = accumulate(pagerank.new_pr.begin(), pagerank.new_pr.end(), 0.0);
            for (i = 0; i < pagerank.num_of_vertex; i++) {
                //pagerank.pr[i] = pagerank.new_pr[i] /sum1;
                if (pagerank.num_outgoing[i] == 0) {
                    dangling_pr += pagerank.new_pr[i];
                }   
            }
        }
        //cout << "start calc" << endl;
        Pagerank::calc_pagerank_value(pagerank.start1,pagerank.end1,dangling_pr,0.0);
        //cout << "end calc" << endl;
        //cout << pagerank.message.size()<<endl;
        if(pagerank.num_of_server!=1){
            myrdma1.rdma_comm("write", pagerank.message);
            Pagerank::combine_pr();
        }

        bool z = fabs(pagerank.diff - prev_diff) < 0.0000001;

        cout.precision(numeric_limits<double>::digits10);
        cout << pagerank.diff<<endl;  //<< " " << prev_diff << " = " << z <<endl;

        if(pagerank.diff < 0.00001 || z){//fabs(pagerank.diff - prev_diff) <0.0000001){
            break;
        }
        prev_diff = pagerank.diff;
        pagerank.pr = pagerank.new_pr;

    }
    
}

string Pagerank::max_pr(){
    int important = 0;
    string result = "";
    long double important_pr = pagerank.pr[0];
    long double tmp = important_pr;
    double sum1 = accumulate(pagerank.pr.begin(), pagerank.pr.end(), 0.0);
    for (int i=0;i< pagerank.num_of_vertex;i++){
        important_pr = max(important_pr, pagerank.pr[i]);
        if(tmp != important_pr){
            important = i;
            tmp = important_pr;
        }
    }

    cout << "important page is " << important << " and value is " << tmp << endl;

    result += to_string(important);
    return result;
}

void Pagerank::init_connection(const char* ip, string server[], int number_of_server, int Port)
{
    myrdma1.initialize_rdma_connection(ip,server,number_of_server,Port,send_buffer,recv_buffer);
    myrdma1.create_rdma_info();
    myrdma1.send_info_change_qp();
    pagerank.num_of_server = number_of_server;
    pagerank.diff = 1;
    for(int i=0;i<number_of_server;i++){
        if(ip == server[i]){
            pagerank.start1 = pagerank.num_of_vertex/number_of_server*i;
            pagerank.end1 = pagerank.start1 + pagerank.num_of_vertex/number_of_server;
        }
        if(ip == server[number_of_server-1]){
            pagerank.end1 = pagerank.num_of_vertex;
        }
    }
    cout << pagerank.start1 << " " <<pagerank.end1 <<endl;
}

void Pagerank::print_pr(){
    size_t i;
    double sum = 0;
    double sum1 = accumulate(pagerank.pr.begin(), pagerank.pr.end(), 0.0);
    cout.precision(numeric_limits<double>::digits10);
    for(i=0;i<pagerank.num_of_vertex;i++){
        cout << "pr[" <<i<<"]: " << pagerank.pr[i] <<endl;
        sum += pagerank.pr[i];
    }
    cerr << "s = " <<round(sum) << endl;
}