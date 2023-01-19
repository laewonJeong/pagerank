#include "pagerank.hpp"
#include "../../includes/network/myRDMA.hpp"
#include "../../includes/network/tcp.hpp"

TCP tcp1;
myRDMA myrdma1;
Pagerank pagerank;
vector<int> sock_idx;
vector<long double> old_pr;
static std::mutex mutx;
double diff = 1;
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
    //bool ret1 = false;
    size_t max_dim = max(from, to);

    if (pagerank.graph.size() <= max_dim) {
        max_dim = max_dim + 1;
        
        pagerank.graph.resize(max_dim);
        pagerank.outgoing.resize(max_dim);
        if (pagerank.num_outgoing.size() <= max_dim) {
            pagerank.num_outgoing.resize(max_dim,0);
        }
    }

    ret = insert_into_vector(pagerank.graph[to], from);
    //ret1 = insert_into_vector(pagerank.outgoing[to],from);
    if (ret) {
        pagerank.num_outgoing[from]++;
        //cout << from << ": " <<pagerank.num_outgoing[from] <<endl;
    }
    //cout << from << ": " <<pagerank.num_outgoing[from] <<endl;
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
            if(line_num%1000000 == 0)
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

    pagerank.pr.resize(pagerank.num_of_vertex, 0);
    pagerank.new_pr = pagerank.pr;
    pagerank.pr[0] = 1;
    

    cout << "Done" <<endl;
}

void Pagerank::thread_calc_pr(int i, double x, double y){
    double tmp = 0;
    
    for(int j = 0; j<pagerank.graph[i].size();j++){
        tmp += df*(pagerank.pr[pagerank.graph[i][j]]/pagerank.num_outgoing[pagerank.graph[i][j]]);
    }
    pagerank.new_pr[i] = stod(to_string((1-df)/pagerank.num_of_vertex + tmp));
    diff += fabs(pagerank.new_pr[i] - pagerank.pr[i]);
    //cout << pagerank.new_pr[i] <<endl;    
}

void Pagerank::calc_pagerank_value(int start, int end, double x, double y){
    diff = 0;
    double tmp;
    ostringstream s;
    for(int i=start;i<end;i++){
        tmp = 0;
        for(int j = 0; j<pagerank.graph[i].size();j++){
            tmp += df*(pagerank.pr[pagerank.graph[i][j]]/pagerank.num_outgoing[pagerank.graph[i][j]]);
        }
        pagerank.new_pr[i] = (1-df)/pagerank.num_of_vertex + tmp;
        //string temp = to_string(i) + " " + to_string(pagerank.new_pr[i]) + "\n";
        //cout << "start rdma_comm"<< endl;
        s << pagerank.new_pr[i];
        pagerank.message += to_string(i) + " " + s.str() + "\n";

        diff += fabs(pagerank.new_pr[i] - pagerank.pr[i]);
    }
    
}

void Pagerank::combine_pr(){
    
    string from, to;
    double d;
    int f;
    for(int i=0;i<3;i++){
        vector<string> a;
        string tmp(pagerank.recv_buffer[i]);
        a = split(tmp,'\n');
        for(int j=0;j<a.size();j++){
            size_t pos = a[j].find(" ");
            from = a[j].substr(0,pos);
            to = a[j].substr(pos+1);
            istringstream ( to ) >> d;
            istringstream (from) >> f;
            pagerank.new_pr[f] = d;
            diff += fabs(pagerank.new_pr[f] - pagerank.pr[f]);  
            //diff += fabs(pagerank.pr[stoi(from)] - old_pr[stoi(from)]);
        }
    }
    //message ="";
    /*for(int i = 0;i<pagerank.num_of_vertex;i++){
        cout << "pr[" <<i<<"]: " << pagerank.pr[i] <<endl;
    }*/
    
}
void Pagerank::send_recv_pagerank_value(int start, int end){
    /*string message = "";
    for(int i=start;i<end;i++){
        message = message + to_string(i)+" " + to_string(pagerank.new_pr[i]) + "\n";
    }*/
    myrdma1.rdma_comm("write", pagerank.message);
}
void Pagerank::run_pagerank(int iter){
    double prev_diff =0;
    int step;
    /*int *clnt_socks = tcp1.client_sock();
    for(int idx=0; idx < 6; idx++){
        if(clnt_socks[idx]!=0){
            sock_idx.push_back(idx);
        }
    }*/
    cout << "progressing..." << endl;
    for(int step =0; step < iter ;step++){
        cout <<"====="<< step+1 << " step=====" <<endl;
        pagerank.message = "";
        Pagerank::calc_pagerank_value(pagerank.start1,pagerank.end1,0.0,0.0);
        Pagerank::send_recv_pagerank_value(pagerank.start1,pagerank.end1);
        Pagerank::combine_pr();
        cout << diff <<endl;
        if(diff < 0.00001 || fabs(diff - prev_diff) <0.000001){
            break;
        }
        prev_diff = diff;
        pagerank.pr = pagerank.new_pr;
    }
    /*for(step =0;step<pagerank.num_of_vertex;step++){
        cout << "pr[" <<step<<"]: " << pagerank.pr[step] <<endl; 
    }*/
    /*vector<size_t>::iterator ci;// current incoming
    
    size_t i;
    double sum_pr; // sum of current pagerank vector elements
    double dangling_pr; // sum of current pagerank vector elements for dangling
    			// nodes
    unsigned long num_iterations = 0;

    size_t num_rows = pagerank.num_of_vertex;

    pagerank.pr.resize(num_rows);

    pagerank.pr[0] = 1;

    while (diff > 0.00001 && num_iterations < iter) {

        sum_pr = 0;
        dangling_pr = 0;
        
        for (size_t k = 0; k < pagerank.pr.size(); k++) {
            double cpr = pagerank.pr[k];
            sum_pr += cpr;
            if (pagerank.num_outgoing[k] == 0) {
                dangling_pr += cpr;
            }
        }

        if (num_iterations == 0) {
            old_pr = pagerank.pr;
        } else {
            // Normalize so that we start with sum equal to one 
            for (i = 0; i < pagerank.pr.size(); i++) {
                old_pr[i] = pagerank.pr[i] / sum_pr;
            }
        }

        
         //After normalisation the elements of the pagerank vector sum to one
        
        sum_pr = 1;
        
        // An element of the A x I vector; all elements are identical 
        double one_Av = 0.85 * dangling_pr / num_rows;

        // An element of the 1 x I vector; all elements are identical 
        double one_Iv = (1 - 0.85) * sum_pr / num_rows;

        // The difference to be checked for convergence 
        string message;
        diff = 0;
        for (i = 0; i < pagerank.num_of_vertex; i++) {
            // The corresponding element of the H multiplication 
            message = "";
            double h = 0.0;
            for (ci = pagerank.graph[i].begin(); ci != pagerank.graph[i].end(); ci++) {
                // The current element of the H vector 
                double h_v = (pagerank.num_outgoing[*ci])
                    ? 1.0 / pagerank.num_outgoing[*ci]
                    : 0.0;
                h += h_v * old_pr[*ci];
            }
            h *= 0.85;
            pagerank.pr[i] = h + one_Av + one_Iv;
            diff += fabs(pagerank.pr[i] - old_pr[i]);
            
            message = message + to_string(i)+ " " + to_string(pagerank.pr[i]);
            myrdma1.rdma_comm("write", message);
            //cout << message << endl;

            //cout << "start send" << endl;
            string from, to;
            for(int j=0;j<3;j++){
                string a(pagerank.recv_buffer[j]);
                size_t pos = a.find(" ");
                from = a.substr(0,pos);
                to = a.substr(pos+1);
                pagerank.pr[stoi(from)] = stod(to);
            }
            //cout << "end send" << endl;
        }
        //Pagerank::send_recv_pagerank_value(pagerank.start1,pagerank.end1);
        //Pagerank::combine_pr();
        //Pagerank::calc_pagerank_value(pagerank.start1, pagerank.end1, one_Av, one_Iv);

    }*/
    for(int i=0;i<pagerank.num_of_vertex;i++){
        cout << "pr[" <<i<<"]: " << pagerank.pr[i] <<endl;
    }
    cout << "Done" << endl;
}

string Pagerank::max_pr(){
    int important = 0;
    string result = "";
    long double important_pr = pagerank.pr[0];
    long double tmp = important_pr;
    for (int i=0;i< pagerank.num_of_vertex;i++){
        important_pr = max(important_pr, pagerank.pr[i]);
        if(tmp != important_pr){
            important = i;
            tmp = important_pr;
        }
    }
    result = result + to_string(important) + " " + to_string(tmp);
    return result;
}

void Pagerank::init_connection(const char* ip, string server[], int number_of_server, int Port)
{
    myrdma1.initialize_rdma_connection(ip,server,number_of_server,Port,pagerank.send_buffer,pagerank.recv_buffer);
    myrdma1.create_rdma_info();
    myrdma1.send_info_change_qp();
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