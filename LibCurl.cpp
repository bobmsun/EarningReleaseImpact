
#include "LibCurl.hpp"

size_t write_data(void *ptr, int size, int nmemb, FILE *stream)
{
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}


void *myrealloc(void *ptr, size_t size)
{
    /* There might be a realloc() out there that doesn't like reallocing
     NULL pointers, so we take care of it here */
    if (ptr)
        return realloc(ptr, size);
    else
        return malloc(size);
}


size_t write_data2(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;
    
    mem->memory = (char *)myrealloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory) {
        memcpy(&(mem->memory[mem->size]), ptr, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }
    return realsize;
}


string getTimeinSeconds(string Time)
{
    std::tm t = {0};
    std::istringstream ssTime(Time);
    char time[100];
    memset(time, 0, 100);
    if (ssTime >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S"))
    {
        sprintf (time, "%ld", mktime(&t));
        return string(time);
    }
    else
    {
        cout << "Parse failed\n";
        return "";
    }
}


// Fetch historical price data from Yahoo Finance
int GetPriceData_Libcurl(map<string, Stock> &myMap, map<string, Stock>::iterator start, map<string, Stock>::iterator end)
{
    vector<double> price;
    vector<string> symbolList;
    vector<vector<string>> startEndDatesList;
    
    for (map<string, Stock>::iterator itr = start; itr != end; itr++)
    {
        symbolList.push_back(itr->first);
        startEndDatesList.push_back(itr->second.GetStartEndDates());
    }
    
    string startTime;
    string endTime;
    vector<string>::iterator itr_symbol = symbolList.begin();
    vector<vector<string>>::iterator itr_date = startEndDatesList.begin();
    map<string, Stock>:: iterator itr_map = start;
    
    struct MemoryStruct data;
    data.memory = NULL;
    data.size = 0;
    
    //file pointer to create file that store the data
    FILE *fp;
    
    //name of files
    const char outfilename[FILENAME_MAX] = "Output.txt";
    const char resultfilename[FILENAME_MAX] = "Results.txt";
    
    /* declaration of an object CURL */
    CURL *handle;
    
    /* result of the whole process */
    CURLcode result;
    
    /* the first functions */
    /* set up the program environment that libcurl needs */
    curl_global_init(CURL_GLOBAL_ALL);
    
    /* curl_easy_init() returns a CURL easy handle that you're gonna reuse in other easy functions*/
    handle = curl_easy_init();
    
    /* if everything's all right with the easy handle... */
    if (handle)
    {
        string sCookies, sCrumb;
        if (sCookies.length() == 0 || sCrumb.length() == 0)
        {
            fp = fopen(outfilename, "w");
            //curl_easy_setopt(handle, CURLOPT_URL, "https://finance.yahoo.com/quote/AMZN/history?p=AMZN");
            curl_easy_setopt(handle, CURLOPT_URL, "https://finance.yahoo.com/quote/AMZN/history");
            
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0);
            curl_easy_setopt(handle, CURLOPT_COOKIEJAR, "cookies.txt");
            curl_easy_setopt(handle, CURLOPT_COOKIEFILE, "cookies.txt");
            
            curl_easy_setopt(handle, CURLOPT_ENCODING, "");
            curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
            result = curl_easy_perform(handle);
            fclose(fp);
            
            /* Check for errors */
            if (result != CURLE_OK)
            {
                /* if errors have occurred, tell us what is wrong with 'result'*/
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
                return 1;
            }
            
            curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data2);
            curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&data);
            
            /* perform, then store the expected code in 'result'*/
            result = curl_easy_perform(handle);
            
            /* Check for errors */
            if (result != CURLE_OK)
            {
                /* if errors have occured, tell us what is wrong with 'result'*/
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
                return 1;
            }
            
            char cKey[] = "CrumbStore\":{\"crumb\":\"";
            char *ptr1 = strstr(data.memory, cKey);
            char *ptr2 = ptr1 + strlen(cKey);
            char *ptr3 = strstr(ptr2, "\"}");
            if ( ptr3 != NULL )
                *ptr3 = NULL;
            
            sCrumb = ptr2;
            
            fp = fopen("cookies.txt", "r");
            char cCookies[100];
            if (fp) {
                while (fscanf(fp, "%s", cCookies) != EOF);
                fclose(fp);
            }
            else
                cerr << "cookies.txt does not exists" << endl;
            
            sCookies = cCookies;
        }
       
        while(true)
        {
            free(data.memory);
            data.memory = NULL;
            data.size= 0;
            
            if (itr_symbol == symbolList.end() or itr_date == startEndDatesList.end() or itr_map == end)
                break;
            
            string urlA = "https://query1.finance.yahoo.com/v7/finance/download/";
            string symbol = *itr_symbol;
            string urlB = "?period1=";
            string urlC = "&period2=";
            string urlD = "&interval=1d&events=history&crumb=";
            
            startTime = getTimeinSeconds((*itr_date)[0] + "T16:00:00");
            endTime = getTimeinSeconds((*itr_date)[1] + "T16:00:00");
            
            string url = urlA + symbol + urlB + startTime + urlC + endTime + urlD + sCrumb;
            const char * cURL = url.c_str();
            const char * cookies = sCookies.c_str();
            curl_easy_setopt(handle, CURLOPT_COOKIE, cookies);   // Only needed for 1st stock
            curl_easy_setopt(handle, CURLOPT_URL, cURL);
            fp = fopen(resultfilename, "ab");
            curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
            result = curl_easy_perform(handle);
            fclose(fp);
            
            /* Check for errors */
            if (result != CURLE_OK)
            {
                /* if errors have occurred, tell us what is wrong with 'result'*/
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
                return 1;
            }
            curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data2);
            curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&data);
            result = curl_easy_perform(handle);
            
            /* Check for errors */
            if (result != CURLE_OK)
            {
                /* if errors have occurred, tell us what is wrong with 'result'*/
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
                return 1;
            }
            stringstream sData;
            sData.str(data.memory);
            string line;
            getline(sData, line);
            
            price.clear();
            for (; getline(sData, line); )
            {
                line = line.erase(line.find_last_of(','));
                line = line.substr(line.find_last_of(',')+1);
                
                try {
                    price.push_back(stod(line));
                }
                catch (...) {
                    continue;
                }
            }
            
            itr_map->second.SetPrice(price);
            
            itr_symbol++;
            itr_date++;
            itr_map++;
            
            cout << "-";
        }
        
        free(data.memory);
        data.size= 0;
        
    }
    else
    {
        fprintf(stderr, "Curl init failed!\n");
        return 1;
    }
    
    /* cleanup since you've used curl_easy_init */
    curl_easy_cleanup(handle);
    
    /* this function releases resources acquired by curl_global_init() */
    curl_global_cleanup();
    
    return 0;
}

