#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <regex>
using namespace std;
typedef pair<vector<string>, vector<string>> pV;
unsigned int pe[0x270];//道具池种子序列
unsigned int iter_num = 0;//道具池遍历指标
unsigned int hx_iter[0x271];
float treasure_weight[] = { 1, 1, 1, 1, 1,  //subtype 1
1, 1, 1, 1, 1,
1, 1, 1, 1, 1,
1, 1, 1, 0.25, 1,
1, 1, 0.01, 0.1, 1,
1, 1, 1, 1, 1,
1, 1, 0.25, 0.25, 0.25,
0.25
};

float devil_weight[] = {  //24种 已知joker会忽略diff=0的恶魔房
1, 1, 1, 1, 1,  //subtype 0 
0.5, 1, 1, 1, 1,
1, 1, 1, 1, 0.5,
0.1, 0.1, 0.1, 0.25,0.25,
1, 0.5, 0.5
};
float angel_weight[] = { //21
	1,1,1,0.5,0.5,
	1,1,0.5,1,1,
	0.5,0.5,0.25,0.25,0.1,
	0.5,1,1,0.5,1,
	1


};


unsigned int p4[16];// theroem:0xE~14 stage seed
unsigned int p2[16];// theroem:0xE~14 start seed
unsigned int p3[16];// theroem:0xE~14 stage seed
pV vec_item;
pV vec_item_shop;
pV vec_item_boss;
pV vec_item_devil;
pV vec_item_angel;

int treasure_count[] = { //宝物所在的位置
	1,2,2,3,4,
	4,2,2,19,3,
	5,3,5,7,9,
	4,1,1,3,2,
	2,2,0,5,21,//这个t[23]是个双宝物房
	7,1,21,12,14,
	4,8,1,5,3,
	4
};

int devil_count[][4] = { //恶魔宝物所在的位置 100=3个乞丐 200=5个红箱子
	{4,5},{4,5},{4,5,6},{4},{4,6},
	{0},{5},{5},{8,10},{10,11},
{7,9},{6,7},{4,5},{7},{0},
{100},{10,11,12,13},{200},{4,6,7},{10,12,14},
{5},{29,30},{10,12}
};

int angel_count[][4]{ //300=白箱子
	{2},{4,5},{5},{0},{2,3,4},
	{10},{6,7}, {1,4,11,14},{9},{4},
	{4,300,300},{3,4},{8,9},{5,6},{5,6,7},
	{7.9},{6,7},{8},{41},{4,5},
	{5}
};


int shift(unsigned int num, unsigned int s1, unsigned int s2, unsigned int s3) {
	unsigned int eax = num;
	unsigned int edx;
	eax = num >> s1;
	eax ^= num;
	edx = eax;
	edx = edx << s2;
	edx ^= eax;
	eax = edx;
	eax = eax >> s3;
	eax ^= edx;
	return eax;
}

int clac_num(float f, float weight[], int length) {//第几个房间
	float passby = 0;
	float total = 0;
	for (int i = 0; i < length; i++) {
		total += weight[i];
	}
	float des = f * total;
	for (int i = 0; i < length; i++) {
		passby += weight[i];
		if (passby > des) {
			return i;
			break;
		}
	}
	return length;

}

vector<string> CalcCollectible(float x, vector<string>vec_name, vector<string>vec2_w) {
	vector<string> trie;//只重复计算两次

	int len = vec2_w.size();
	//TreasureRoom到18f就截止 
	float total = 0;
	float* weightcount = new float[len];
	for (unsigned int i = 0; i < len; i++) {
		total += atof(vec2_w[i].c_str());
		weightcount[i] = atof(vec2_w[i].c_str());
	}
	float weightcur = x * total;
	float weightpass = 0;
	unsigned int item_count = 0;

	for (unsigned int i = 0; i < 3; i++) {
		while (weightcur >= weightpass) {
			weightpass += weightcount[item_count];
			if (weightpass > weightcur) {
				break;
			}
			item_count++;
		}
		trie.push_back(vec_name[item_count].c_str());
		weightcur = (weightpass - weightcur) * total / weightcount[item_count];
		weightpass = 0;
		item_count = 0;
	}
	return trie;
}


void read_file(const string& filename, string& filedata)
{
	ifstream infile;
	char* fileBuffer = NULL;
	infile.open(filename, ios::in);
	if (infile.is_open())
	{
		infile.seekg(0, ios::end);
		auto len = infile.tellg();  //获取文件长度
		infile.seekg(0, ios::beg);  //设置读取位置为起始位置

		fileBuffer = new char[(size_t)len + 1];
		memset(fileBuffer, 0, (size_t)len + 1);
		infile.read(fileBuffer, len);
		filedata = fileBuffer;
		delete[] fileBuffer;
	}

	infile.close();
}

float get_roomfloat(unsigned int rdseed) {
	float rdf = 0;
	unsigned int rng = shift(rdseed, 0x1, 0x15, 0x14);
	unsigned int y = 0x2f7ffffe;
	unsigned long long offs = 0x0;
	if ((rng >> 0x1f) == 0x1) {
		offs = 0x41f0000000000000;
	}
	__asm {
		movd xmm0, rng
		cvtdq2pd xmm0, xmm0
		addsd xmm0, qword ptr ds : offs
		cvtpd2ps xmm0, xmm0
		mulss xmm0, dword ptr ds : y
		movss dword ptr ss : rdf, xmm0
	}
	return rdf;
}
unsigned int stringtoseed(string seed)
{
	const string chars = "ABCDEFGHJKLMNPQRSTWXYZ01234V6789";
	//string seed = "WXQVJS2A";
	//string seed = "AW01EGKY";
	unsigned int num = 0;
	for (int i = 0; i < 6; i++) {
		unsigned int pos = chars.find(seed[i]);
		num += (pos & 0x1F);
		if (i != 5) {
			num = num << 5;
		}
	}
	num = num << 2;
	unsigned int tnum = 0;//用来搞最后两位的东西
	unsigned int x;
	unsigned int p1 = chars.find(seed[6]);
	unsigned int p2 = chars.find(seed[7]);
	tnum = (p1 & 0x1F);
	num += (tnum >> 3) & 0x3;
	num ^= 0x0FEF7FFD;
	cout << hex << num << endl;
	return num;
}

void ReGenPe() {//重新生成pe和更新hx_iter
	for (int i = 1; i < 0x270; i++) {//进一步生成
		if (i - 1 < 0xe3) {
			unsigned int h1 = hx_iter[i];
			unsigned int h1_cur = hx_iter[i - 1];
			h1 ^= h1_cur;
			h1 &= 0x7FFFFFFF;
			h1 ^= h1_cur;
			h1_cur = h1;
			h1 >>= 1;
			h1_cur &= 0x1;
			if (h1_cur == 0x1) {
				h1 ^= 0x9908b0df;

			}
			h1 ^= hx_iter[i - 1 + 397];
			hx_iter[i - 1] = h1;
			//rva = 701888  固定为9908b0df
		}
		else if (i - 1 < 0x26f) {
			unsigned int h1 = hx_iter[i];
			unsigned int h1_cur = hx_iter[i - 1];
			h1 ^= h1_cur;
			h1 &= 0x7FFFFFFF;
			h1 ^= h1_cur;
			h1_cur = h1;
			h1 >>= 1;
			h1_cur &= 1;
			if (h1_cur == 0x1) {
				h1 ^= 0x9908b0df;
			}
			h1 ^= hx_iter[i - 1 - 227];
			hx_iter[i - 1] = h1;
		}

	}
	unsigned int ecx = hx_iter[0x26f];
	unsigned int eax;
	ecx ^= hx_iter[0];
	ecx &= 0x7fffffff;
	ecx ^= hx_iter[0x26f];
	eax = ecx;
	ecx >>= 1;
	eax &= 1;
	if (eax == 0x1) {
		eax = 0x9908b0df;
	}
	eax ^= hx_iter[0x18C];
	eax ^= ecx;
	hx_iter[0x26f] = eax;
	hx_iter[0x270] = 0;
	//eax != 270 and eax ==270 公用部分

	for (int i = 1; i < 0x271; i++) {
		unsigned int p1 = hx_iter[0x270];//eax
		unsigned int p2 = hx_iter[i - 1];//ecx
		p1++;
		hx_iter[0x270] = p1;
		p1 = p2;
		p1 >>= 0xB;
		p2 ^= p1;
		p1 = p2;
		p1 &= 0xff3a58ad;
		p1 <<= 0x7;
		p2 ^= p1;
		p1 = p2;
		p1 &= 0xffffdf8c;
		p1 <<= 0xf;
		p2 ^= p1;
		p1 = p2;
		p1 >>= 0x12;
		p1 ^= p2;
		pe[i - 1] = p1;
	}

}

float gen_float(unsigned int used1_gen) {
	unsigned int final = shift(used1_gen, 0x5, 0x9, 0x7);
	unsigned int finalgen = shift(final, 0x5, 0x9, 0x7);
	unsigned int finalseed = shift(finalgen, 0x1, 0xb, 0x6);
	float x;
	unsigned int x2;
	unsigned int y = 0x2f7ffffe;
	unsigned long long offs = 0x0;
	if ((finalseed >> 0x1f) == 0x1) {
		offs = 0x41f0000000000000;
	}
	__asm {
		movd xmm0, finalseed
		cvtdq2pd xmm0, xmm0
		addsd xmm0, qword ptr ds : offs
		cvtpd2ps xmm0, xmm0
		mulss xmm0, dword ptr ds : y
		movss dword ptr ss : x, xmm0
		movss dword ptr ss : x2, xmm0
	}
	return x;
}

pV LoadXmlStruct(string tre)
{
	string itempool;
	//
	string itempool_pos = "itempools.xml";
	//string itempool_pos = "./itempools.xml";
	read_file(itempool_pos, itempool);
	int r = itempool.find('\t');
	while (r != string::npos)
	{
		if (r != string::npos)
		{
			itempool.replace(r, 1, "");
			r = itempool.find('\t');
		}
	}
	itempool.erase(std::remove(itempool.begin(), itempool.end(), '\n'), itempool.end());
	string St = "<Pool Name=\"" + tre + "\">";
	int pos1 = itempool.find(St, 0);
	int pos2 = itempool.find("</Pool>", pos1);
	string cut = itempool.substr(pos1, pos2 - pos1);
	regex re("Id=\"(.+?)\"");
	regex re_w("Weight=\"(.+?)\"");
	regex re_d("DecreaseBy=\"(.+?)\"");
	regex re_r("RemoveOn=\"(.+?)\"");
	regex re_name("<!--(.+?)-->");
	regex re2("(\\d+)");
	regex re2_2("[0-9](.?)([0-9]{0,3})(?=\")");
	std::sregex_token_iterator mt(cut.begin(), cut.end(), re);
	std::sregex_token_iterator mt2(cut.begin(), cut.end(), re_w);
	std::sregex_token_iterator mtname(cut.begin(), cut.end(), re_name);
	std::sregex_token_iterator end;
	std::vector<std::string> vec;
	std::vector<std::string> vec_w;
	std::vector<std::string> vec_name;
	while (mt != end) {
		vec.push_back(*mt++);
		vec_w.push_back(*mt2++);
		vec_name.push_back(*mtname++);
	}
	std::vector<std::string> vec2;
	std::vector<std::string> vec2_w;
	auto fir = vec.begin();
	auto fir2 = vec_w.begin();
	auto ed = vec.end();
	auto ed2 = vec2.end();
	while (fir != ed)
	{
		std::sregex_token_iterator mt2((*fir).begin(), (*fir).end(), re2);
		std::sregex_token_iterator mt2_2((*fir2).begin(), (*fir2).end(), re2_2);
		while (mt2 != end) {
			vec2.push_back(*mt2++);
			vec2_w.push_back(*mt2_2++);;
		}
		fir++;
		fir2++;
	}
	int len = vec2.size();
	for (unsigned int i = 0; i < len - 1; i++) { //混淆部分，i需要后续记录
		unsigned int hx = pe[iter_num] % (len - i);
		if (hx != len - i - 1) {//je
			string vectemp = vec2[len - i - 1];
			string vectemp2 = vec2[hx];
			string vec_weight = vec2_w[len - i - 1];
			string vectempname = vec_name[len - i - 1];
			vec2[hx] = vectemp;
			vec2[len - i - 1] = vectemp2;
			vec2_w[len - i - 1] = vec2_w[hx];
			vec2_w[hx] = vec_weight;//交换权重
			vec_name[len - i - 1] = vec_name[hx];
			vec_name[hx] = vectempname;
		}
		iter_num++;
		if (iter_num == 0x270) {
			ReGenPe(); //
			iter_num = 0;
		}
	}

	return { vec2_w,vec_name };
}
vector <unsigned int> gen_collectibleseed(unsigned int seed, int RoomType, int count[][4]) {
	vector<unsigned int> res;

	for (int i = 0; i < 4; i++) {
		unsigned seedgen = seed;
		if (count[RoomType][i] == 0) {
			res.push_back(0x999);
			return res;
		}
		if (count[RoomType][i] == 300) {
			res.push_back(300);
			return res;
		}
		for (int j = 0; j < count[RoomType][i]; j++) {
			seedgen = shift(seedgen, 0x2, 0x7, 0x7);
		}
		res.push_back(seedgen);
	}
	return res;
}
void gen_res(unsigned int stage, unsigned int pos_treasure) {
	unsigned int ps[75];
	ps[0] = p4[stage];//p4[1]=Sp3[1]=SSp2[1]=SS+Shift(p,0x3,0x17,0x19),p2=stageseed,p4=stageseed两次next;
	for (int i = 1; i < 75; i++) {
		ps[i] = shift(ps[i - 1], 0x5, 0x9, 0x7);
	}
	unsigned int devil = shift(p2[stage], 0x1, 0x5, 0x13);//恶魔房种子
	unsigned int devils[15];
	devils[0] = devil;
	for (int i = 1; i < 15; i++) {
		devils[i] = shift(devils[i - 1], 0x1, 0x5, 0x13);
	}

	unsigned int used1 = shift(ps[pos_treasure], 0x2, 0x7, 0x9);//应该是宝箱房的种子 变化规律需要寻找
	float roomtype = get_roomfloat(ps[pos_treasure]);//生成宝箱房种类的float
	int rec_num = clac_num(roomtype, treasure_weight, 36);//计算哪个房间
	unsigned int used1_gen1 = used1; //这个有用
	//计算恶魔房 注意joker房的房间种类由devil0x4决定
	int length = sizeof(devil_count) / sizeof(devil_count[0]);
	float roomtype_joker = get_roomfloat(devils[0x4]);
	int rec_num_joker = clac_num(roomtype_joker, devil_weight, length);
	vector<unsigned int> collectibleseed_joker = gen_collectibleseed(devils[0x2], rec_num_joker, devil_count);
	float roomtype_bossdevil = get_roomfloat(devils[0x8]);
	int rec_num_bossdev = clac_num(roomtype_bossdevil, devil_weight, length);
	vector<unsigned int> collectibleseed_bossdevil = gen_collectibleseed(devils[0x6], rec_num_bossdev, devil_count);
	//
	//天使房
	int lengthA = sizeof(angel_count) / sizeof(angel_count[0]);
	int rec_num_jokerA = clac_num(roomtype_joker, angel_weight, lengthA);
	vector<unsigned int> collectibleseed_jokerA = gen_collectibleseed(devils[0x2], rec_num_jokerA, angel_count);
	int rec_num_bossA = clac_num(roomtype_bossdevil, angel_weight, lengthA);
	vector<unsigned int> collectibleseed_bossA = gen_collectibleseed(devils[0x6], rec_num_bossA, angel_count);
	//
	//unsigned int used1_gen2 = shift(used1_gen1,0x5,0x9,0x7); //这个有用
	//
	used1 = shift(used1, 0x2, 0x7, 0x9); //这个用来生成9e803750
	//
	//unsigned int clue = shift(used1, 0x2, 0x15, 0x9);
	//unsigned int clue2 = shift(clue, 0x2, 0x15, 0x9);
	//unsigned int used2 = shift(used1, 0x2, 0x7, 0x9);
	//unsigned int used3 = shift(used2, 0xb, 0x7, 0xc);
	//
	unsigned int used1_gen;
	used1_gen = shift(used1, 0x2, 0x7, 0x7);
	int TimeToTheItem = treasure_count[rec_num];
	for (int i = 1; i < TimeToTheItem; i++) {
		used1_gen = shift(used1_gen, 0x2, 0x7, 0x7);//how many times remains unknown
	}
	//
	unsigned int recd = used1_gen;
	unsigned int recd2 = shift(recd, 0x1, 0x15, 0x14);
	//
	unsigned int used_list[25];
	used_list[0] = used1;
	//
	/*for (int i = 1; i < 25; i++) {
		used_list[i] = shift(used_list[i - 1], 0x2, 0x7, 0x7);
	}*/
	//used1_gen = shift(used1_gen, 0x2, 0x7, 0x7);
	//used1_gen = shift(used1_gen, 0x2, 0x7, 0x7);//关键是要知道做了多少次这个玩意
	//



	float x = gen_float(used1_gen);

	//混淆序列


	//hx_iter[0] = 0x734f8be3; //这个还需要求出
	unsigned int itempool_calcseed1 = p2[0xf];//用来计算道具池的种子 就是上面的东西 
	itempool_calcseed1 = shift(itempool_calcseed1, 0x3, 0x17, 0x19); //afa89375
	unsigned itempool_2 = shift(itempool_calcseed1, 0x1, 0x9, 0x1d);
	unsigned itempool_f = itempool_2;
	//之后执行1f次
	for (unsigned int cons = 0; cons < 0x1f; cons++) {
		itempool_f = shift(itempool_f, 0x1, 0x9, 0x1d);
		itempool_f = shift(itempool_f, 0x1, 0x9, 0x1d);
	}
	hx_iter[0] = itempool_f;
	//cout << itempool_f;
	for (int i = 1; i < 0x270; i++) {
		unsigned int shr = hx_iter[i - 1] >> 0x1e;
		shr ^= hx_iter[i - 1];
		hx_iter[i] = shr * 0x6c078965;
		hx_iter[i] += i;

	}
	for (int i = 1; i < 0x270; i++) {//进一步生成
		if (i - 1 < 0xe3) {
			unsigned int h1 = hx_iter[i];
			unsigned int h1_cur = hx_iter[i - 1];
			h1 ^= h1_cur;
			h1 &= 0x7FFFFFFF;
			h1 ^= h1_cur;
			h1_cur = h1;
			h1 >>= 1;
			h1_cur &= 0x1;
			if (h1_cur == 0x1) {
				h1 ^= 0x9908b0df;

			}
			h1 ^= hx_iter[i - 1 + 397];
			hx_iter[i - 1] = h1;
			//rva = 701888  固定为9908b0df
		}
		else if (i - 1 < 0x26f) {
			unsigned int h1 = hx_iter[i];
			unsigned int h1_cur = hx_iter[i - 1];
			h1 ^= h1_cur;
			h1 &= 0x7FFFFFFF;
			h1 ^= h1_cur;
			h1_cur = h1;
			h1 >>= 1;
			h1_cur &= 1;
			if (h1_cur == 0x1) {
				h1 ^= 0x9908b0df;
			}
			h1 ^= hx_iter[i - 1 - 227];
			hx_iter[i - 1] = h1;
		}

	}
	unsigned int ecx = hx_iter[0x26f];
	unsigned int eax;
	ecx ^= hx_iter[0];
	ecx &= 0x7fffffff;
	ecx ^= hx_iter[0x26f];
	eax = ecx;
	ecx >>= 1;
	eax &= 1;
	if (eax == 0x1) {
		eax = 0x9908b0df;
	}
	eax ^= hx_iter[0x18C];
	eax ^= ecx;
	hx_iter[0x26f] = eax;
	hx_iter[0x270] = 0;
	//eax != 270 and eax ==270 公用部分

	for (int i = 1; i < 0x271; i++) {
		unsigned int p1 = hx_iter[0x270];//eax
		unsigned int p2 = hx_iter[i - 1];//ecx
		p1++;
		hx_iter[0x270] = p1;
		p1 = p2;
		p1 >>= 0xB;
		p2 ^= p1;
		p1 = p2;
		p1 &= 0xff3a58ad;
		p1 <<= 0x7;
		p2 ^= p1;
		p1 = p2;
		p1 &= 0xffffdf8c;
		p1 <<= 0xf;
		p2 ^= p1;
		p1 = p2;
		p1 >>= 0x12;
		p1 ^= p2;
		pe[i - 1] = p1;
	}
	//读取itempool:
	if (stage == 0x1) {
		vec_item = LoadXmlStruct("treasure");
		vec_item_shop = LoadXmlStruct("shop");
		vec_item_boss = LoadXmlStruct("boss");
		vec_item_devil = LoadXmlStruct("devil");
		vec_item_angel = LoadXmlStruct("angel");
	}

	vector<string>  vec2_w = vec_item.first;
	vector<string>  vec_name = vec_item.second;
	vector<string>  vec2_devil_w = vec_item_devil.first;
	vector<string>  vec_devil_name = vec_item_devil.second;
	vector<string>  vec2_angel_w = vec_item_angel.first;
	vector<string>  vec_angel_name = vec_item_angel.second;
	vector<string> trie = CalcCollectible(x, vec_name, vec2_w);
	cout << "stage:" << stage << endl;
	cout << "Treasure:" << endl;
	cout << "Try1:" << trie[0].c_str() << "\t";
	cout << "Try2:" << trie[1].c_str() << "\t";
	cout << "Try3:" << trie[2].c_str() << "\t" << endl;
	cout << "JokerDevil:" << endl;
	int lengthD = collectibleseed_joker.size();
	int lengthD2 = collectibleseed_bossdevil.size();
	for (int i = 0; i < lengthD; i++) {
		if (collectibleseed_joker[0] == 0x999) {
			cout << "None" << endl;;
			break;
		}
		if (i != 0 && collectibleseed_joker[i] == 0x999) {
			break;
		}
		cout << "POS:" << i + 1 << "\t";
		float xd = gen_float(collectibleseed_joker[i]);
		vector<string> trieJoker = CalcCollectible(xd, vec_devil_name, vec2_devil_w);
		cout << "Try1:" << trieJoker[0].c_str() << "\t";
		cout << "Try2:" << trieJoker[1].c_str() << "\t";
		cout << "Try3:" << trieJoker[2].c_str() << endl;
	}
	cout << "BossDevilRoom:" << endl;
	for (int i = 0; i < lengthD2; i++) {
		if (collectibleseed_bossdevil[0] == 0x999) {
			cout << "None" << endl;;
			break;
		}
		if (i != 0 && collectibleseed_bossdevil[i] == 0x999) {
			break;
		}
		cout << "POS:" << i + 1 << "\t";
		float xd = gen_float(collectibleseed_bossdevil[i]);
		vector<string> trieJoker = CalcCollectible(xd, vec_devil_name, vec2_devil_w);
		cout << "Try1:" << trieJoker[0].c_str() << "\t";
		cout << "Try2:" << trieJoker[1].c_str() << "\t";
		cout << "Try3:" << trieJoker[2].c_str() << endl;
	}
	cout << "JokerAngel:" << endl;
	int lengthDA = collectibleseed_jokerA.size();
	int lengthD2A = collectibleseed_bossA.size();
	for (int i = 0; i < lengthDA; i++) {
		if (collectibleseed_jokerA[0] == 0x999) {
			cout << "None" << endl;;
			break;
		}
		if (i != 0 && collectibleseed_jokerA[i] == 0x999) {
			break;
		}
		if (collectibleseed_jokerA[i] == 300) {
			cout << "White Chest" << "\t";
			continue;
		}
		cout << "POS:" << i + 1 << "\t";
		float xd = gen_float(collectibleseed_jokerA[i]);
		vector<string> trieJokerA = CalcCollectible(xd, vec_angel_name, vec2_angel_w);
		cout << "Try1:" << trieJokerA[0].c_str() << "\t";
		cout << "Try2:" << trieJokerA[1].c_str() << "\t";
		cout << "Try3:" << trieJokerA[2].c_str() << endl;
	}
	cout << "BossAngelRoom:" << endl;
	for (int i = 0; i < lengthD2A; i++) {
		if (collectibleseed_bossA[0] == 0x999) {
			cout << "None" << endl;;
			break;
		}
		if (i != 0 && collectibleseed_bossA[i] == 0x999) {
			break;
		}
		if (collectibleseed_bossA[i] == 300) {
			cout << "White Chest" << "\t";
			continue;
		}
		cout << "POS:" << i + 1 << "\t";
		float xd = gen_float(collectibleseed_bossA[i]);
		vector<string> trieJokerA = CalcCollectible(xd, vec_angel_name, vec2_angel_w);
		cout << "Try1:" << trieJokerA[0].c_str() << "\t";
		cout << "Try2:" << trieJokerA[1].c_str() << "\t";
		cout << "Try3:" << trieJokerA[2].c_str() << endl;
	}
}

int main()
{
	cout << "*****Repentance Seed Calculator_V0.0.2*****" << endl;

	while (1) {
		iter_num = 0;
		string seed;
		cout << "Seed:";
		cin >> seed;
		unsigned int p = stringtoseed(seed);
		//unsigned int p= 0x0a2a1910;
		//unsigned int p = 0xd0bc504d;
		//unsigned int p = 0x9b32cbbe;//startseed,万物之源
		//devils0x6?也许是Boss进入的恶魔房的 0x2为Joker


		//p = shift(p, 5, 9, 7);
		//p = shift(p, 5, 9, 7);

		p = shift(p, 0x3, 0x17, 0x19);//need more iter
		p2[0] = p; p3[0] = p; p4[0] = p; //p2 的0xf是用于生成道具池的
		for (int i = 1; i < 16; i++) {
			p2[i] = shift(p2[i - 1], 0x3, 0x17, 0x19);
			p3[i] = shift(p2[i], 0x5, 0x9, 0x7);
			p4[i] = shift(p3[i], 0x5, 0x9, 0x7);
		}
		for (int i = 1; i <= 6; i++) {

			gen_res(i, 0xa);
			//gen_res(i, 0x);
		}
	}
}


//fac88819 boss room 在stageseed之后约8个为treasure room
//85E8B42A treasure room seed ,改变这个以后treasure room 宝物改变。