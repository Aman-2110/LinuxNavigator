/*** header files ***/

#include <bits/stdc++.h>
#include <termios.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <ftw.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <wchar.h>

#define moveCursor(x, y) cout<<"\033["<<(x)<<";"<<(y)<<"H";

using namespace std;

/*** Global Variables ***/

struct termios orig_termios;
struct winsize w;

stack<string> prevDir, nextDir;

int cursorPosition, row, col, maxUserNameSize, maxGroupNameSize;
bool inNormalMode;

vector<string> filesList;
vector<string> fileSize;
vector<string> filePermissions;
vector<string> userName;
vector<string> groupName;
vector<string> date;
string dirName, command;

/*** For printing files ***/

void printDetails(){
   printf("\033c");
   int size = filesList.size();
   if(size != 0){
      vector<string> cursor(size, "   ");
      cursor[cursorPosition] = ">>>";
      string entry, shortEntry;
      int printLimit = row - 2;
      if(size <= printLimit){
         for(int i = 0 ; i < size ; i++){
            if(i == cursorPosition){
               cout << "\033[1;31m";
            }
            shortEntry = cursor[i] + " " + filePermissions[i] + " " + userName[i] + " " + fileSize[i] + " " + filesList[i];
            entry = cursor[i] + " " + filePermissions[i] + " " + userName[i] + " " + groupName[i] + " " + fileSize[i] + " " + date[i] + " " + filesList[i];
            if(col < 42 + maxUserNameSize + maxGroupNameSize){
               cout << shortEntry.substr(0,col - 3) << endl;
            }else{
               cout << entry.substr(0,col - 3) << endl;
            }
            if(i == cursorPosition){
               cout << "\033[0m";
            }
         }
      }else{
         int startIndex = max(cursorPosition - printLimit + 1, 0);
         
         for(int i = startIndex ; i < startIndex + printLimit; i++){
            if(i == cursorPosition){
               cout << "\033[1;31m";
            }
            shortEntry = cursor[i] + " " + filePermissions[i] + " " + userName[i] + " " + fileSize[i] + " " + filesList[i];
            entry = cursor[i] + " " + filePermissions[i] + " " + userName[i] + " " + groupName[i] + " " + fileSize[i] + " " + date[i] + " " + filesList[i];
            if(col < 42 + maxUserNameSize + maxGroupNameSize){
               cout << shortEntry.substr(0,col - 3) << endl;
            }else{
               cout << entry.substr(0,col - 3) << endl;
            }
            if(i == cursorPosition){
               cout << "\033[0m";
            }
         }
      }
   }else{
      cout << "\033[1;31mNo Files Found\033[0m";
   }
     
   
   string status = "Normal Mode ::: " + dirName;
   moveCursor(row - 1, 0);
   if(inNormalMode){
      status = "Normal Mode ::: " + dirName ;
      cout << status.substr(0, col - 3) << endl;
   }else{
      status = "Command Mode ::: " + dirName ;
      cout << status.substr(0, col - 3) << endl;
   }
}

/*** For converting File Size From B to KB/MB/GB ***/

string convertFileSizeInUnit(long long int size){
   if(size < 1024){
      return to_string(size);
   }else if(size >= 1024 && size < 1048576){
      return to_string(size/1024) + "K";
   }else if(size >= 1048576 && size < 1073741824){
      return to_string(size/(1024*1024)) + "M";
   }else{
      return to_string(size/(1024*1024*1024)) + "G";
   }
}

/*** reset and clear file ***/

void resetAndClear(){
   cursorPosition = 0;
   maxUserNameSize = 0;
   maxGroupNameSize = 0;
   filePermissions.clear();
   userName.clear();
   groupName.clear();
   fileSize.clear();
   date.clear();
   filesList.clear(); 
}

/*** converting length of all string equal ***/

void resizeString(){
   for(int i = 0 ; i < filesList.size() ; i++){
      userName[i].insert(0, maxUserNameSize - userName[i].size() , ' ');
      groupName[i].insert(0, maxGroupNameSize - groupName[i].size() , ' ');
      fileSize[i].insert(0, 5 - fileSize[i].size() , ' ');
   }
}

/*** For fetching files details in the given Directory ***/

void fetchFilesDetails(const char * dirName) {
   resetAndClear();
   string directory(dirName);
   DIR *dr;
   struct dirent *en;
   dr = opendir(dirName); 
   if (dr) {
      while ((en = readdir(dr)) != NULL) {
         string str(en->d_name);
         filesList.push_back(str);
      }
      closedir(dr);
   }
   
   if(filesList.size() == 0)
      return;
   // Sorting File Names in a lexicographical order
   sort(filesList.begin(), filesList.end(), [](const string& a, const string& b) -> bool {
      for (size_t c = 0; c < a.size() and c < b.size(); c++) {
         if (tolower(a[c]) != tolower(b[c]))
               return (tolower(a[c]) < tolower(b[c]));
      }
      return a.size() < b.size();
   });

   for(auto str : filesList){
      
      string filePath = directory + "/" + str;
      
      // using stat struct to fetch details of file
      struct stat fileStat;
      if(stat(filePath.c_str(), &fileStat) < 0)
         return;

      //storing file size in vector
      fileSize.push_back(convertFileSizeInUnit(fileStat.st_size));

      //using getpwuid and getgrgid to fetch username and groupname respectively
      struct passwd *pw = getpwuid(fileStat.st_uid);
      struct group  *gr = getgrgid(fileStat.st_gid);

      string user(pw->pw_name);
      string group(gr->gr_name);
      
      if(maxUserNameSize < user.size())
         maxUserNameSize = user.size();

      if(maxGroupNameSize < group.size())
         maxGroupNameSize = group.size();

      userName.push_back(pw->pw_name);
      groupName.push_back(gr->gr_name);

      //using ctime method for fetching last modified time of file
      string lastTimeModified = ctime(&fileStat.st_mtime);
      date.push_back(lastTimeModified.substr(4,12)); 

      //storing filePermission
      string filePermission = S_ISDIR(fileStat.st_mode) ? "d" : "-";
      filePermission += (fileStat.st_mode & S_IRUSR) ? "r" : "-";
      filePermission += (fileStat.st_mode & S_IWUSR) ? "w" : "-";
      filePermission += (fileStat.st_mode & S_IXUSR) ? "x" : "-";
      filePermission += (fileStat.st_mode & S_IRGRP) ? "r" : "-";
      filePermission += (fileStat.st_mode & S_IWGRP) ? "w" : "-";
      filePermission += (fileStat.st_mode & S_IXGRP) ? "x" : "-";
      filePermission += (fileStat.st_mode & S_IROTH) ? "r" : "-";
      filePermission += (fileStat.st_mode & S_IWOTH) ? "w" : "-";
      filePermission += (fileStat.st_mode & S_IXOTH) ? "x" : "-";

      filePermissions.push_back(filePermission);
   }
   resizeString();
}

/*** terminal ***/

void die(const char * s) {
   cerr << s << "\n";
   exit(1);
}

void disableRawMode() {
   if (tcsetattr(STDIN_FILENO, TCSANOW, & orig_termios) == -1)
      die("tcsetattr");
}

void enableRawMode() {
   if (tcgetattr(STDIN_FILENO, & orig_termios) == -1) die("tcgetattr");
   atexit(disableRawMode);
   struct termios raw = orig_termios;
   raw.c_lflag &= ~(ECHO | ICANON);
   if (tcsetattr(0, TCSANOW, &raw) == -1) die("tcsetattr");
}

/*** to open file ***/

void openFile(const char *fileName){
   if(fork() == 0){
      execl("/usr/bin/xdg-open", "xdg-open", fileName, (char *)0);
      exit(1);
   }
}

/*** to open directory ***/

void openDirectory(){
   if(filesList[cursorPosition] == ".") return;
   else if(filesList[cursorPosition] == ".."){
      if(dirName == "/") {
         cursorPosition = 0;
         printDetails();
         return;
      }
      while(!nextDir.empty()) nextDir.pop();
      prevDir.push(dirName);
      while(dirName.back() != '/')  dirName.pop_back();
      dirName.pop_back();
   }else{
      prevDir.push(dirName);
      if(dirName == "/")   dirName += filesList[cursorPosition];
      else  dirName += "/" + filesList[cursorPosition];
   }
   if(dirName == "") dirName = "/";
   fetchFilesDetails(dirName.c_str());
   printDetails();
}

/*** to direct to home directory ***/

void openHomeDirectory(){
   string homeDir(getenv("HOME"));
   if(dirName == homeDir)
      return;
   while(!nextDir.empty()) nextDir.pop();
   prevDir.push(dirName);
   dirName = homeDir;
   fetchFilesDetails(dirName.c_str());
   printDetails();
}

/*** for navigation ***/

void ArrowKeyPressed(){
   enableRawMode();
   char arr = getchar();
   disableRawMode();
   // UP ARROW
   if(arr == 'A'){
      if (cursorPosition != 0){
         cursorPosition--;
         printDetails();
      }
   }
   // DOWN ARROW
   else if(arr == 'B'){
      if (cursorPosition != filesList.size() - 1){
         cursorPosition++;
         printDetails();
      }
   }
   // RIGHT ARROW
   else if(arr == 'C'){
      if(!nextDir.empty()){
         prevDir.push(dirName);
         dirName = nextDir.top();
         nextDir.pop();
         fetchFilesDetails(dirName.c_str());
         printDetails();
      }
   }
   // LEFT ARROW
   else if(arr == 'D'){
      if(!prevDir.empty()){
         nextDir.push(dirName);
         dirName = prevDir.top();
         prevDir.pop();
         fetchFilesDetails(dirName.c_str());
         printDetails();  
      }
   }
}

/*** to direct to parent directory ***/

void backSpacePressed(){
   if(dirName == "/")   return;
   while(!nextDir.empty()) nextDir.pop();
   prevDir.push(dirName);
   while(dirName.back() != '/')  dirName.pop_back();
   
   dirName.pop_back();
   if(dirName == "") dirName = "/";
   
   fetchFilesDetails(dirName.c_str());
   printDetails();
}

void do_resize(int num){
   ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
   row = w.ws_row;
   col = w.ws_col;
   command = "";
   printDetails();
}

/*** command mode ***/

bool gotFile;

/*** to convert relative path to absolute ***/

string getAbsolutePath(string s){
   if(s[0] == '~')
      return getenv("HOME") + s.substr(1);
   if(s[0] != '/')
      s = dirName + "/" + s;
   if(!realpath(s.c_str(), NULL))
      return "";
   s = string(realpath(s.c_str(), NULL));
   return s;
}

/*** to get file name ***/

string getFileName(string s){
   string res = "";
   for(int i = s.size() - 1; i >= 0 ; i--){
      if(s[i] == '/')
         break;
      res = s[i] + res;
   }
   return res;
}

/*** to search file in given directory ***/

void searchFileInDirectory(string directoryName, string fileName){
   DIR* dir = opendir(directoryName.c_str());
   if (dir == NULL) {
      return;
   }

   struct dirent* entity;
   entity = readdir(dir);
   while (entity != NULL) {
      if(entity->d_name == fileName){
         gotFile = true;
         return;
      }
      if (entity->d_type == DT_DIR && strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0) {
         string subFolderPath = directoryName + "/" + entity->d_name;
         searchFileInDirectory(subFolderPath, fileName);
         if(gotFile)
            break;
      }
      entity = readdir(dir);
   }
   closedir(dir);
}

/*** to delete file ***/

static int removeFiles(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb)
{
   if(pathname == ".." || pathname == ".")
      return 0;

   if(remove(pathname) < 0)
      return -1;

   return 0;
}

/*** to copy file ***/

bool copyFile(string src, string dst){

   struct stat st;
   if (stat(src.c_str(), &st) == -1){
      cout << "\033[1;31mCannot open file\033[0m";
      return false;
   }

   ifstream source(src, ios::binary);
   ofstream dest(dst, ios::binary);

   dest << source.rdbuf();

   source.close();
   dest.close();
   chown(&dst[0], st.st_uid, st.st_gid);
   chmod(&dst[0], st.st_mode);
   return true;
}

/*** to copy folder ***/

bool copyFolder(string src, string dest){

   DIR *direc;
   struct dirent *d;
   int ret = mkdir(dest.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
   direc = opendir(src.c_str());
   if (direc == NULL){
      return false;
   }

   bool b=true;
   while ((d = readdir(direc)) != NULL){
      if (strcmp(d->d_name,".") && strcmp(d->d_name,"..")){
         string name = d->d_name;
         string eachfile = src + "/" + d->d_name;
         struct stat st;
         if (stat(eachfile.c_str(), &st) == -1)
               return false;

         if (S_ISDIR(st.st_mode))
               b = b && copyFolder(eachfile, dest + "/" + d->d_name);
         else
               b = b && copyFile(eachfile, dest + "/" + d->d_name);
      }
   }
   return b;
}

void switchCommandMode(){
   
   while(true)
   {
      command = "";
      char ch;
      
      enableRawMode();
      ch = getchar();
      disableRawMode();

      printDetails();

      while(ch != 10 && ch != 27)
      {  
         
         if(ch == 127 && command.size() > 0)
         {
            command.pop_back();
            cout << '\b' << " " << "\b";
         }else if(ch != 127){
            cout << ch;
            command += ch;
         }         
         enableRawMode();
         ch = getchar();
         disableRawMode();
      }

      if(ch == 27){
         inNormalMode = true;
         printDetails();
         return;
      }

      printDetails();

      if(command == "quit"){
         printf("\033c");
         exit(0);
      }

      if(command == "")
         continue;

      vector<string> vec;
      char *ptr = strtok(&command[0], " "); 

      while(ptr != NULL){
         vec.push_back(ptr);
         ptr = strtok(NULL, " ");
      }
      // search
      if(vec[0] == "search"){
         if(vec.size() != 2){
            cout << "\033[1;31mInvalid Syntax :: required -> search <file_name> || search <dir_name>\033[0m\n";
            continue;
         }

         gotFile = false;
         searchFileInDirectory(dirName, vec[1]);

         if(gotFile)
            cout << "True" << endl;
         else
            cout << "False" << endl;
      }
      // rename
      else if(vec[0] == "rename"){
         if(vec.size() != 3){
            cout << "\033[1;31minvalid syntax :: required -> rename <old_filename> <new_filename>\033[0m\n";
            continue;
         }

         string source = getAbsolutePath(vec[1]);
         string destination = getAbsolutePath(vec[2]);
   
         if (rename(source.c_str(), destination.c_str()) != 0){
            cout << "\033[1;31mFile does not exists\033[0m\n";
         }else{
            fetchFilesDetails(dirName.c_str());
            printDetails();
            cout << "File renamed successfully"<<endl;
         }

      }
      // delete_file
      else if(vec[0] == "delete_file"){
         if(vec.size() != 2){
            cout << "\033[1;31mInvalid Syntax :: required -> delete_file <file_path>\033[0m\n";
            continue;
         }
         
         string source = getAbsolutePath(vec[1]);
         string destination = string(getenv("HOME")) + "/.local/share/Trash/files/" + getFileName(source);

         if (rename(source.c_str(), destination.c_str()) != 0){
            cout << "\033[1;31mFile does not exists\033[0m\n";
         }else{
            fetchFilesDetails(dirName.c_str());
            printDetails();
            cout << "File deleted successfully"<<endl;
         }
      }
      // create_file
      else if(vec[0] == "create_file"){
         if(vec.size() != 3){
            cout << "\033[1;31minvalid syntax :: required -> create_file <file_name> <destination_path>\033[0m\n";            
            continue;
         }
         string location = getAbsolutePath(vec[2]);
         location += '/' + vec[1];

         if(open(location.c_str(), O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < -1){
            cout << "\033[1;31mError creating file\n\033[0m";
         }          
         else{
            fetchFilesDetails(dirName.c_str());
            printDetails();
            cout << "File created successfully" << endl;
         }
      }
      // create_dir
      else if(vec[0] == "create_dir"){
         if(vec.size() != 3){
            cout << "\033[1;31minvalid syntax :: required -> create_dir <dir_name> <destination_path>\033[0m";            
            continue;
         }
         string location = getAbsolutePath(vec[2]);

         if(mkdir((location + "/" + vec[1]).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1){
            cout << "\033[1;31mDestination does not exists\033[0m\n";            
         }else{
            fetchFilesDetails(dirName.c_str());
            printDetails();
            cout << "Directory Created Succesfully!";
         }
      }
      // goto
      else if(vec[0] == "goto"){
         if(vec.size() != 2){
            cout << "\033[1;31minvalid syntax :: required -> goto <dir_name>\033[0m\n";
            continue;
         }
         string location = getAbsolutePath(vec[1]);
         if(dirName == location){
            cout << "\033[1;31mAlready in directory\033[0m\n";
            continue;
         }else if(location == ""){
            cout << "\033[1;31mDestination not exists\033[0m";            
            continue;
         }
         while(!nextDir.empty()) nextDir.pop();
         prevDir.push(dirName);
         dirName = location;
         fetchFilesDetails(dirName.c_str());
         printDetails();
      }
      // delete_dir
      else if(vec[0] == "delete_dir"){
         if(vec.size() != 2){
            cout << "\033[1;31minvalid syntax :: required -> delete_dir <dir_path>\n\033[0m";           
            continue;
         }
         if (nftw(getAbsolutePath(vec[1]).c_str(), removeFiles, 10, FTW_DEPTH|FTW_MOUNT|FTW_PHYS) < 0){
            printDetails();
            cout << "\033[1;31mError Deleting files\033[0m\n";
            continue;
         }
         fetchFilesDetails(dirName.c_str());
         printDetails();
         cout << "Directory deleted successfully"<<endl;

      }
      // copy
      else if(vec[0] == "copy"){
         int n = vec.size();
         if (n < 3){
            cout << "\033[1;31minvalid syntax :: required -> copy <source_file(s)> <destination_directory>\n\033[0m";            
            continue;
         }
         string dest = vec[n-1];
         string destnPath = getAbsolutePath(dest);
         vector<string> cp_f;
         bool cp_s=false;
         for (int i=1; i<n-1; i++){
            
            string sourcePath = getAbsolutePath(vec[i]);

            struct stat st;
            bool is_folder=false;
            if (stat(sourcePath.c_str(), &st) == -1){
               cp_f.push_back(vec[i]);
               continue;
            }
            else{
               if ((S_ISDIR(st.st_mode)))
                  is_folder=true;
            }
            string s=vec[i].substr(vec[i].find_last_of('/')+1);
            if (is_folder == false){
               bool cp_stat = copyFile(sourcePath, destnPath+"/"+s);
               if (!cp_stat)
                  cp_f.push_back(vec[i]);
               cp_s = cp_s || cp_stat;
            }
            else{ 
               gotFile = false;
               searchFileInDirectory(&sourcePath[0],destnPath.substr(destnPath.find_last_of('/')+1));
               if (gotFile){
                  cout << "\033[1;31mCannot copy a folder into itself!\n\033[0m";                  
                  gotFile = false;
                  cp_f.push_back(vec[i]);
                  continue;
               } else {
                  bool cp_stat = copyFolder(sourcePath, destnPath+"/"+s);
                  if (!cp_stat)
                     cp_f.push_back(vec[i]);
                  cp_s = cp_s || cp_stat;
               }
            }
         }
         fetchFilesDetails(dirName.c_str());
         printDetails();
         if (cp_f.size()==0){
            if (cp_s)
                  cout<<"Copied Successfully";
            else{
               cout << "\033[1;31mNo file/directory to copy\n\033[0m";
            }
         } else {
            string failed="";
            for (int f=0;f<cp_f.size();f++)
                  failed+=(cp_f[f]+", ");
            failed.pop_back();
            failed.pop_back();
            if (failed.length()+12<w.ws_col){
               cout << "\033[1;31mCannot copy "<<failed << "\033[0m\n";
            }                  
            else{
               cout << "\033[1;31mCannot copy "<<failed.substr(0,w.ws_col-12) << "\n\033[0m";
            }                  
         }
      }
      // move
      else if(vec[0] == "move"){
         if (vec.size()<3){
            cout << "\033[1;31minvalid syntax :: move <source_file(s)> <destination_directory>\n\033[0m";
         }            
         else{
            string dest = getAbsolutePath(vec[vec.size()-1]);
            vector<string> mv_f;
            bool mv_s=false;
            for (int i=1;i<vec.size()-1;i++){
               string src=getAbsolutePath(vec[i]);
               struct stat st;
               if (stat(src.c_str(), &st) == -1){
                  mv_f.push_back(vec[i]);
               }
               else{
                  string s=dest+"/"+src.substr(src.find_last_of('/')+1);
                  if ((S_ISDIR(st.st_mode))){
                     gotFile = false;
                     searchFileInDirectory(&src[0],dest.substr(dest.find_last_of('/')+1));
                     if (gotFile){
                        moveCursor(w.ws_row, 0);
                        cout << "\033[1;31mCannot move a folder into itself!\n\033[0m";
                        mv_f.push_back(vec[i]);
                        gotFile = false;
                        continue;
                     } else {
                        bool mv_stat = copyFolder(src, s);
                        if (mv_stat){
                           if (nftw(src.c_str(), removeFiles,10, FTW_DEPTH|FTW_MOUNT|FTW_PHYS) < 0){
                              cout << "\033[1;31mFile Tree walk error\n\033[0m";
                              mv_f.push_back(vec[i]);
                              continue;
                           }
                           mv_s=true;
                        } else
                           mv_f.push_back(vec[i]);
                     }
                  }
                  else{
                     int ret = mkdir(dest.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                     int check = rename(src.c_str(), s.c_str());
                     if (check == -1)
                           mv_f.push_back(vec[i]);
                     else
                           mv_s=true;
                  }
               }
            }
            fetchFilesDetails(dirName.c_str());
            printDetails();
            if (mv_f.size()==0){
                  if (mv_s)
                     cout<<"Moved Successfully\n";
                  else{
                     cout << "\033[1;31mNo file/directory to move\n\033[0m";
                  }
            } else {
                  string failed="";
                  for (int f=0;f<mv_f.size();f++)
                     failed+=(mv_f[f]+", ");
                  failed.pop_back();
                  failed.pop_back();
                  if (failed.length()+12<w.ws_col){
                     cout << "\033[1;31mCannot move "<<failed<< "\n\033[0m";
                  }
                  else{
                     cout << "\033[1;31mCannot move "<<failed.substr(0,w.ws_col-12) << "\n\033[0m";
                  }
            }
         }
      }
   }
}

/*** init ***/

int main() {
   inNormalMode = true;

   prevDir = stack<string>();
   nextDir = stack<string>();
   
   ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
   row = w.ws_row;
   col = w.ws_col;

   dirName = filesystem::current_path();

   fetchFilesDetails(dirName.c_str());
   printDetails();

   signal(SIGWINCH, do_resize);

   while (1) {
      
      char c;
      enableRawMode();
      c = getchar();
      disableRawMode();

      // arrow key
      if(c == '['){
         ArrowKeyPressed();
      }
      
      // ENTER KEY
      else if (c == 10){
         if(filePermissions[cursorPosition][0] == 'd')
            openDirectory();
         else{
            string fileName = dirName + "/" + filesList[cursorPosition];
            openFile(fileName.c_str());
         }         
      }
      // q pressed for termination of program
      else if (c == 'q'){ 
         printf("\033c");  
         exit(0); 
      }
      // h pressed for going to home directory
      else if (c == 'h')  
         openHomeDirectory();
      // Backspace pressed for moving to parent directory
      else if(c == 127)   
         backSpacePressed();
      // Colon for command mode
      else if(c == 58){
         inNormalMode = false;
         printDetails();
         switchCommandMode();
      }
   }
   return 0;
}