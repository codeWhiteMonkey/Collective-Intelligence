#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
using namespace std;

struct sim_Node
{
	int ID;
	float score;
	bool operator <(sim_Node tmp)
	{
		if(score > tmp.score)
			return true;
		else if(score == tmp.score)
			if(ID < tmp.ID)
				return true;
		return false;
	}
};
//电影信息 movieId movieInfor
vector<string> movies;
//用户字典 personId movieId score
vector<vector<int>> dict;
//用户评价的电影数目
vector<int> user_count;
void loadData()
{
	/************************************************************************/
	/* 读取u.data
	/* 数据格式 用户id\t电影id\t评分\t时间 
	/************************************************************************/
	ifstream fin("..\\movielens\\u.data");
	if(fin.fail())
	{
		cout << "open u.data error" << endl;
		return;
	}
	string str;
	size_t userId;
	size_t movieId;
	size_t point; 
	while (!fin.eof())
	{
		getline(fin, str);
		if(str.length() == 0)
			break;
		//cout << str << endl;
		int index = str.find('\t');
		userId = atoi(str.substr(0, index).c_str());
		int last = index+1;
		index = str.find('\t', last);
		movieId = atoi(str.substr(last, index-last).c_str());
		last = index + 1;
		index = str.find('\t', last);
		point = atoi(str.substr(last, index-last).c_str());

		if(dict.size() <= userId)
			dict.resize(userId+1);
		if(dict[userId].size() <= movieId)
			dict[userId].resize(movieId+1);
		dict[userId][movieId] = point;
		if(user_count.size() <= userId)
			user_count.resize(userId+1);
		user_count[userId]++;
	}
	fin.clear();
	fin.close();
	//读取u.data结束
	/************************************************************************/
	/* 读取u.item
	/* 数据格式 电影id|电影名字|时间|。。。。
	/************************************************************************/
	fin.open("..\\movielens\\u.item");
	if(fin.fail())
	{
		cout << "open u.item error" << endl;
		return;
	}
	string movieInfo;
	//string str;
	while(!fin.eof())
	{
		getline(fin, str);
		int index = str.find("|");
		movieId = atoi(str.substr(0, index).c_str());
		if(movies.size() <= movieId)
			movies.resize(movieId+1);
		int last = index+1;
		index = str.find("|", last);
		movieInfo = str.substr(last, index-last);
		movies[movieId] = movieInfo;
	}
	fin.close();
	//读取u.item结束
}

void getCommonMovie(const size_t p1, const size_t p2, vector<int> &result)
{
	
	for (size_t m1 = 1; m1 < dict[p1].size(); m1++)
	{
		if(dict[p1][m1] == 0)
			continue;
		if(m1 < dict[p2].size() && dict[p2][m1] > 0)
		{
			result.push_back(m1);
		}
	}
}
//根据欧拉距离公式，返回两人的相似度
float sim_distance(size_t p1, size_t p2)
{
	float d;
	d = 0;
	vector<int> commonMovies;
	getCommonMovie(p1, p2, commonMovies);
	for (size_t i = 0; i < commonMovies.size(); i++)
	{
		float tmp_d = dict[p1][commonMovies[i]] - dict[p2][commonMovies[i]];
		d += tmp_d*tmp_d;
	}
	d = sqrt(d);
	return 1/(1+d);

}
//根据的皮尔逊系数公式，返回两人的相似度
float sim_pearson(size_t p1, size_t p2)
{
	vector<int> commonMovies;
	getCommonMovie(p1, p2, commonMovies);
	//两人没有共同之处
	int N = commonMovies.size();
	if (N == 0)
	{
		return 1;
	}
	float sum1, sum2, sum12, sum1_sq, sum2_sq;
	sum1 = sum2 = sum12 = sum1_sq = sum2_sq = 0;
	
	for (int i = 0; i < N; i++)
	{
		size_t movieId = commonMovies[i];
		sum1 += dict[p1][movieId];
		sum2 += dict[p2][movieId];
		sum12 += dict[p1][movieId]*dict[p2][movieId];
		sum1_sq += dict[p1][movieId]*dict[p1][movieId];
		sum2_sq += dict[p2][movieId]*dict[p2][movieId];
	}
	float mol = sum12 - sum1*sum2*(1/N);
	float den = sqrt( (sum1_sq- sum1*sum1*(1/N)) * (sum2_sq - sum2*sum2*(1/N)) );
	if(den == 0)
		return 0;
	return mol/den;
}

//根据的Jaccard系数公式，返回两人的相似度
float sim_jaccard(size_t p1, size_t p2)
{
	vector<int> commonMovies;
	getCommonMovie(p1, p2, commonMovies);
	int N = commonMovies.size();
	int L1, L2;
	L1 = user_count[p1];
	L2 = user_count[p2];
	//cout << p1 << " " << p2 << endl;
	
	if(L1 == 0 || L2 == 0)
		return 0;
	return N*1.0/(L1+L2-N);
}
typedef float (*SIM_FUNC)(size_t, size_t);

//得到相似度最高的n个用户
void topMatch(int p, vector<sim_Node> &result, SIM_FUNC sim_f = sim_distance, int n=5)
{
	for (size_t i = 1; i < dict.size(); i++)
	{
		if(p == i)
			continue;
		float r = sim_f(p, i);
		sim_Node tmpNode;
		tmpNode.ID = i;
		tmpNode.score = r;
		result.push_back(tmpNode);
	}
	sort(result.begin(), result.end());
	result.erase(result.begin()+n, result.end());
}

//基于用户的过滤，得到推荐
void getRecommendations(int p, vector<sim_Node> &result, int n = 10, SIM_FUNC sim_f = sim_pearson)
{
	vector<float> movie_score;
	vector<float> movie_sim;
	for (size_t personId = 1; personId < dict.size(); personId++)
	{
		if(personId == p)
			continue;
		float sim = sim_f(p, personId); //计算两人的相似度
		if(sim <= 0)
			continue;

		//对于persondId评价过的电影
		for (size_t movieId = 1; movieId < dict[personId].size(); movieId++)
		{
			if(dict[personId][movieId] == 0)
				continue;
			if(movieId < dict[p].size() && dict[p][movieId] > 0)//p对movieId电影做过评价
				continue;
			if(movie_score.size() <= movieId)
				movie_score.resize(movieId+1);
			movie_score[movieId] += sim*dict[personId][movieId];//计算得分
			if(movie_sim.size() <= movieId)
				movie_sim.resize(movieId+1);		
			movie_sim[movieId] += sim;//计算总的相似度
		}
	}
	for (size_t i = 1; i < movie_sim.size(); i++)
	{
		if(movie_sim[i] == 0)
			continue;
		sim_Node tmp;
		tmp.ID = i;
		tmp.score = movie_score[i]*1.0/movie_sim[i];
		result.push_back(tmp);
	}
	sort(result.begin(), result.end());
	if(result.size() > n)
		result.erase(result.begin()+n, result.end());
}

int main()
{
	loadData();

	vector<sim_Node> sim_person;
	//topMatch(87, sim_person, sim_pearson);
	getRecommendations(20, sim_person, 15, sim_jaccard);
	cout << "推荐度\t影片" << endl;
	cout << "-------------------------------------------------------" << endl;
	for (int i = 0 ; i < sim_person.size(); i++)
	{
		cout << setiosflags(ios::fixed) << setprecision(2) << sim_person[i].score << "\t"
			<< movies[sim_person[i].ID] << endl;
	}
	//sim_pearson(87, 242);
	return 0;
}