// test for FileUtils::AppendPlaylistData
#include <iostream>
#include <string>
#include <vector>

using namespace std;

string log(vector<char> vec) {
    string ret = "{ ";
    for (int i=0; i<vec.size(); i++)
        ret += string("") + char(vec[i]) + string(", ");
    if (vec.size() > 0)
        ret = ret.substr(0, ret.length()-2);
    ret += " }";
    return ret;
}

int main() {
    vector<char> bkpSongs = {'1','2','3','4','5'}; // hash
    vector<char> bkpSongsData = {'A','B','C','D','E'}; // append data

    auto testFn = [&] (const string &name, const vector<char> &songs, const vector<char> &ans)  {
        vector<char> ret = songs;
        for (int i = 0, j = 0; i < songs.size(); i++) {
            if (j < bkpSongs.size() && songs[i] == bkpSongs[j]) {
                ret[i] = bkpSongsData[j++];
                continue;
            }

            j = min((int)bkpSongs.size()-1, j);
            for (int k1 = j, k2 = j + 1; k1 >= 0 || k2 < bkpSongs.size(); k1--, k2++) {
                if (k1 >= 0 && songs[i] == bkpSongs[k1]) {
                    ret[i] = bkpSongsData[k1];
                    j = k1+1;
                    break;
                }
                if (k2 < bkpSongs.size() && songs[i] == bkpSongs[k2]) {
                    ret[i] = bkpSongsData[k2];
                    j = k2+1;
                    break;
                }
            }
        }
        cout << name << " : ";
        if (ret.size() != ans.size()) {
            cout << "Wrong" << endl;
            cout << "ret: " << log(ret) << endl;
            cout << "ans: " << log(ans) << endl;
            return false;
        }
        for (int i = 0; i < ret.size(); i++)
            if (ret[i] != ans[i]) {
                cout << "Wrong" << endl;
                cout << "ret: " << log(ret) << endl;
                cout << "ans: " << log(ans) << endl;
                return false;
            }
        cout << "Pass" << endl;
        return true;
    };

    cout << "=== Basic cases ===" << endl;
    if (!testFn("Unchange Case", {'1','2','3','4','5'}, {'A','B','C','D','E'})) return -1;
    if (!testFn("Remove Case  ", {'1','2','4','5'}, {'A','B','D','E'})) return -1;
    if (!testFn("Insert Case  ", {'1','2','6','3','4','5'}, {'A','B','6','C','D','E'})) return -1;
    if (!testFn("Change Case 1", {'1','2','4','3','5'}, {'A','B','D','C','E'})) return -1;
    if (!testFn("Change Case 2", {'5','2','3','4','1'}, {'E','B','C','D','A'})) return -1;

    cout << endl;
    cout << "=== Same songs in a playlist ===" << endl;
    bkpSongs = {'1','2','3','4','2'}; // hash
    if (!testFn("Unchange Case", {'1','2','3','4','2'}, {'A','B','C','D','E'})) return -1;
    if (!testFn("Change Case 1", {'1','2','4','3','2'}, {'A','B','D','C','E'})) return -1;
    if (!testFn("Change Case 2", {'2','1','3','4','2'}, {'B','A','C','D','E'})) return -1;
    if (!testFn("Change Case 3", {'1','2','3','2','4'}, {'A','B','C','E','D'})) return -1;

    return 0;
}