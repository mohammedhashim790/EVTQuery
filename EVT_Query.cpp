#include <windows.h>
#include <sddl.h>
#include <stdio.h>
#include <winevt.h>
#include <iostream>
#include<jni.h>
#include "Resolvers_Resolver.h";
#include <string>
#include<vector>
#include<queue>


using namespace std;

#pragma comment(lib, "wevtapi.lib")

const int SIZE_DATA = 4096;
TCHAR XMLDataCurrent[SIZE_DATA];
TCHAR XMLDataUser[SIZE_DATA];

#define ARRAY_SIZE 25
#define TIMEOUT 1000  // 1 second; Set and use in place of INFINITE in EvtNext call

DWORD PrintResults(EVT_HANDLE hResults);
DWORD PrintEvent(EVT_HANDLE hEvent); // Shown in the Rendering Events topic
string PrettyPrint(EVT_HANDLE hResults, EVT_HANDLE hProviderMetadata);
void PrettyPrintAll(EVT_HANDLE hResults, JNIEnv* env, EVT_HANDLE hProviderMetadata);
//, JNIEnv* env
string QueryChannel(WCHAR path[], WCHAR eventId[]);

void QueryChannelAll(WCHAR path[], WCHAR eventId[], JNIEnv* env);



string QueryChannelPaginatedNext(
	string pathQuery,
	int fromEventRecordId
);

string QueryChannelPaginatedPrev(
	string pathQuery,
	int fromEventRecordId
);

string GetEvents(
	string pathQuery
);

void GetAllEvents(
	string pathQuery,
	JNIEnv* env
);


void RefreshEvents(
	string pathQuery,
	int fromEventRecordId
);





void ListChannels();
void GetPublisherName();

LPWSTR GetMessageString(
	EVT_HANDLE hMetadata, 
	EVT_HANDLE hEvent, 
	EVT_FORMAT_MESSAGE_FLAGS FormatId,
	DWORD *dwBufferUsed
);

int TotalEventsInChannel(WCHAR path[]);

void SendToJava(JNIEnv* env, jclass jcls,string toSend);


jclass loadClass(JNIEnv* env) {
	jclass clas = env->FindClass("com/example/evtquery/ListChannels");

	return clas;
}





JNIEXPORT jint JNICALL JNICALL Java_Resolvers_Resolver_Add(JNIEnv* env, jobject thisObj, jint num1, jint num2) {
	cout << "Numbers are : " << num1 << " & " << num2;

	cout << "Sum is : " << (num1 + num2);

	return (num1 + num2);
}

JNIEXPORT jstring JNICALL Java_Resolvers_Resolver_Query(
	JNIEnv* env, 
	jobject thisObj,
	jstring queryPath
) {

	WCHAR *path = (WCHAR*) env->GetStringChars(queryPath, NULL);
	WCHAR eventId[] = L"Event/System";

	return env->NewStringUTF(QueryChannel(path,eventId).c_str());
}


JNIEXPORT jobject JNICALL Java_Resolvers_Resolver_QueryObject(JNIEnv* env, jobject thisObj) {

	string str("Hello from C++");

	return env->NewStringUTF(str.c_str());
}



JNIEXPORT void JNICALL Java_Resolvers_Resolver_QueryChannelsNext(
	JNIEnv* env, 
	jobject thisObj, 
	jstring queryPath,  
	jint fromEventRecordID) {


	string str(env->GetStringUTFChars(queryPath,NULL));

	string eventIdParams("*[System[");
	eventIdParams.append("(EventRecordID>=" +
		to_string(fromEventRecordID) + ")"
		+ "]]");

	cout << "PathQuery" << " " << eventIdParams << endl;
	wstring wEventIdParams(eventIdParams.begin(), eventIdParams.end());

	WCHAR* eventId = (WCHAR*)wEventIdParams.c_str();


	wstring wPathQuery(str.begin(), str.end());

	WCHAR* path = (WCHAR*)wPathQuery.c_str();

	QueryChannelAll(path,eventId,env);
}

JNIEXPORT jstring JNICALL Java_Resolvers_Resolver_QueryChannelsPrev(
	JNIEnv* env, 
	jobject thisObj, 
	jstring queryPath,  
	jint fromEventRecordID) {


	string str(env->GetStringUTFChars(queryPath,NULL));

	return env->NewStringUTF(QueryChannelPaginatedPrev(str,fromEventRecordID).c_str());
}




JNIEXPORT jint JNICALL Java_Resolvers_Resolver_GetTotalLogsFromChannel(JNIEnv* env, jobject thisObj, jstring queryPath) {
	WCHAR* path = (WCHAR*)env->GetStringChars(queryPath, NULL);


	int res = TotalEventsInChannel(path);

	return res;
}



JNIEXPORT jstring JNICALL Java_Resolvers_Resolver_GetEvents(JNIEnv* env, jobject thisObj, jstring queryPath) {
	string query(env->GetStringUTFChars(queryPath, NULL));

	return env->NewStringUTF(GetEvents(query).c_str());
}


vector<string> events;

JNIEXPORT void JNICALL Java_Resolvers_Resolver_FetchAllData(JNIEnv* env, jobject thisObj, jstring queryPath) {
	WCHAR* path = (WCHAR*)env->GetStringChars(queryPath, NULL);
	WCHAR eventId[] = L"Event/System";


	cout << "Initialised java class" << (loadClass(env) != nullptr);

	//SendToJava(env,loadClass(env),"HI");


	QueryChannelAll(path,eventId,env);


}







int main()
{
	//GetPublisherName();


	WCHAR path[] = L"Security";

	string queryPath("Security");

	cout <<  "Total Channels "  << TotalEventsInChannel(path) << "\n\n\n\n";

	//WCHAR eventId[] = L"Event/System";
	WCHAR eventId[] = L"*";



	QueryChannel(path,eventId);



	
}


int TotalEventsInChannel(WCHAR path[]) {


	HANDLE hEvent = OpenEventLog(NULL, path);

	if (hEvent == NULL) {
		return -1;
	}

	DWORD dwTotal;
	if (GetNumberOfEventLogRecords(hEvent, &dwTotal)) {
		return dwTotal;
	}

	return -1;


}

void QueryChannelAll(WCHAR path[], WCHAR eventId[],JNIEnv* env) {
	//WCHAR eventId[] = L"Event/System";
	//WCHAR eventId[] = L"Event/System";
	//WCHAR path[] = L"Security";

	DWORD status = ERROR_SUCCESS;
	EVT_HANDLE hResults = NULL;
	LPWSTR pwsPath = path;
	LPWSTR pwsQuery = eventId;

	LPWSTR pwsMessage = NULL;


	EVT_HANDLE hProviderMetadata = NULL;

	LPWSTR pwszPublisherName = path;

	cout << "Result Started" << endl;

	hProviderMetadata = EvtOpenPublisherMetadata(NULL, pwszPublisherName, NULL, 0, 0);
	if (NULL == hProviderMetadata)
	{
		wprintf(L"EvtOpenPublisherMetadata failed with %d\n", GetLastError());
		goto cleanup;
	}


	hResults = EvtQuery(NULL, pwsPath, pwsQuery, EvtQueryChannelPath);// EvtQueryReverseDirection);
	if (NULL == hResults)
	{
		status = GetLastError();
		if (ERROR_EVT_CHANNEL_NOT_FOUND == status)
			wprintf(L"The channel was not found.\n");
		else if (ERROR_EVT_INVALID_QUERY == status)
			wprintf(L"The query is not valid.\n");
		else
			wprintf(L"EvtQuery failed with %lu.\n", status);

		goto cleanup;
	}
	Sleep(1000);

cleanup:
	PrettyPrintAll(hResults,env ,hProviderMetadata);
}

string QueryChannel(WCHAR path[], WCHAR eventId[]) {
	//WCHAR eventId[] = L"Event/System";
	//WCHAR eventId[] = L"Event/System";
	//WCHAR path[] = L"Security";

	DWORD status = ERROR_SUCCESS;
	EVT_HANDLE hResults = NULL;
	LPWSTR pwsPath = path;
	LPWSTR pwsQuery = eventId;

	LPWSTR pwsMessage = NULL;


	EVT_HANDLE hProviderMetadata = NULL;

	LPWSTR pwszPublisherName = path;

	cout << "Result Started" << endl;

	hProviderMetadata = EvtOpenPublisherMetadata(NULL, pwszPublisherName, NULL, 0, 0);
	if (NULL == hProviderMetadata)
	{
		wprintf(L"EvtOpenPublisherMetadata failed with %d\n", GetLastError());
		goto cleanup;
	}


	hResults = EvtQuery(NULL, pwsPath, pwsQuery, EvtQueryChannelPath);// EvtQueryReverseDirection);
	if (NULL == hResults)
	{
		status = GetLastError();

		if (ERROR_EVT_CHANNEL_NOT_FOUND == status)
			wprintf(L"The channel was not found.\n");
		else if (ERROR_EVT_INVALID_QUERY == status)
			wprintf(L"The query is not valid.\n");
		else
			wprintf(L"EvtQuery failed with %lu.\n", status);

		goto cleanup;
	}
	//Sleep(1000);

cleanup:

	PrintResults(hResults);


	//string res = PrettyPrint(hResults, hProviderMetadata);


	return "";
}


void SendToJava(JNIEnv* env, jclass jcls,string toSend) {

	cout << "Sending to Java : " << endl;


	cout << toSend.length() << endl;

	//jmethodID mid = env->GetStaticMethodID(jcls, "GetEntityFromResolver", "(Ljava/lang/String;)V");  // find method
	jmethodID mid = env->GetStaticMethodID(jcls, "EventReceiver", "([B)V");  // find method
	if (mid == nullptr)
		cerr << "ERROR: method void mymain() not found !" << endl;
	else {
		jbyteArray byteArray = env->NewByteArray(toSend.size());
		env->SetByteArrayRegion(byteArray, 0, toSend.size(), (jbyte*)toSend.data());
		//env->CallStaticVoidMethod(jcls, mid, env->NewStringUTF(json.c_str()));                      // call method
		env->CallStaticVoidMethod(jcls, mid, byteArray);                      // call method
		cout << endl;
	}
}



string GetEvents(
	string pathQuery
) {

	string eventQuery("*[System[TimeCreated[timediff(@SystemTime) <= 600000]]]");


	cout << "PathQuery" << " " << eventQuery << endl;
	wstring wEventIdParams(eventQuery.begin(), eventQuery.end());

	WCHAR* eventId = (WCHAR*)wEventIdParams.c_str();


	wstring wPathQuery(pathQuery.begin(), pathQuery.end());

	WCHAR* path = (WCHAR*)wPathQuery.c_str();


	wprintf(L"%s\n", path);

	return QueryChannel(path, eventId);

}



string QueryChannelPaginatedNext(
	string pathQuery,
	int fromEventRecordId
) {

	string eventIdParams("*[System[");
	eventIdParams.append("(EventRecordID>=" +
		to_string(fromEventRecordId) + ")"
		+ "]]");

	cout << "PathQuery" << " " << eventIdParams<<endl;
	wstring wEventIdParams(eventIdParams.begin(), eventIdParams.end());

	WCHAR *eventId = (WCHAR*) wEventIdParams.c_str();


	wstring wPathQuery(pathQuery.begin(),pathQuery.end());

	WCHAR *path = (WCHAR*) wPathQuery.c_str();

	return QueryChannel(path,eventId);
}

string QueryChannelPaginatedPrev(
	string pathQuery,
	int fromEventRecordId
) {

	string eventIdParams("*[System[");
	eventIdParams.append("(EventRecordID>=" +
		to_string(fromEventRecordId - ARRAY_SIZE) + ")"
		+ "]]");


	cout << "PathQuery" << " " << eventIdParams<<endl;
	wstring wEventIdParams(eventIdParams.begin(), eventIdParams.end());

	WCHAR *eventId = (WCHAR*) wEventIdParams.c_str();


	wstring wPathQuery(pathQuery.begin(),pathQuery.end());

	WCHAR *path = (WCHAR*) wPathQuery.c_str();


	wprintf(L"%s\n",path);


	return QueryChannel(path, eventId);
}


void PrettyPrintAll(
	EVT_HANDLE hResults,
	JNIEnv* env,
	EVT_HANDLE hProviderMetadata) {


	EVT_HANDLE hEvent[ARRAY_SIZE];
	DWORD status = ERROR_SUCCESS;
	DWORD dwReturned = 0;

	DWORD currentDwBufferUsed = 0;


	DWORD dwBufferUsed = 0;



	LPWSTR pwsMessage = NULL;


	cout << "PRetty Print All Started";


	wstring wres;
	string res;



	int count = 0;


	cout << "Pretty Print Processing " << endl;


	//if (!EvtNext(hResults, ARRAY_SIZE, hEvent, INFINITE, 0, &dwReturned))
	//{
	//	wprintf(L"EvtNext failed with %lu\n", status);
	//	goto cleanup;
	//}

	jclass jCls = loadClass(env);


	while (true) {
		
		if (!EvtNext(hResults, ARRAY_SIZE, hEvent, INFINITE, 0, &dwReturned)) {

			if (ERROR_NO_MORE_ITEMS != (status = GetLastError()))
			{
				cout << " Evt Over " << dwReturned << endl;
				break;
			}
		}

		if (dwReturned == 0) {
			break;
		}

		count += dwReturned;
		cout << count << endl;
		
		for (int iter = 0; iter < dwReturned; iter++) {
			pwsMessage = GetMessageString(
				hProviderMetadata,
				hEvent[iter],
				EvtFormatMessageXml,
				&dwBufferUsed
			);

			currentDwBufferUsed += dwBufferUsed;

			char buffer[1000];

			wres.append(pwsMessage);


			dwBufferUsed = 0;

			//wprintf(L"Event ID %d\nResult : \n%s \n", iter, pwsMessage);
		}
		if (wres.size() > 1000) {
			res.append("<EventArray>");
			res.append(string(wres.begin(), wres.end()));
			res.append("</EventArray>");
			SendToJava(env, jCls, res);


			res = "";
			wres.clear();

		}


	}


	cout << count;
	


cleanup:

	if (hEvent)
		EvtClose(hEvent);

	if (hResults)
		EvtClose(hResults);

	if (hProviderMetadata)
		EvtClose(hProviderMetadata);



}



string PrettyPrint(EVT_HANDLE hResults, EVT_HANDLE hProviderMetadata) {

	EVT_HANDLE hEvent[ARRAY_SIZE];
	DWORD status = ERROR_SUCCESS;
	DWORD dwReturned = 0;

	DWORD currentDwBufferUsed = 0;


	DWORD dwBufferUsed = 0;



	LPWSTR pwsMessage = NULL;



	wstring wres;

	string res;


	cout << "Pretty Print Processing " << endl;


	if (!EvtNext(hResults, ARRAY_SIZE, hEvent, INFINITE, 0, &dwReturned))
	{
		wprintf(L"EvtNext failed with %lu\n", status);
		goto cleanup;
	}



	for (int iter = 0; iter < dwReturned; iter++) {
		pwsMessage = GetMessageString(
			hProviderMetadata,
			hEvent[iter],
			EvtFormatMessageXml,
			&dwBufferUsed
		);

		currentDwBufferUsed += dwBufferUsed;

		char buffer[1000];

		wres.append(pwsMessage);


		dwBufferUsed = 0;

		wprintf(L"Event ID %d\nResult : \n%s \n", iter, pwsMessage);


	}

	//Convert Accumulated String to XML String 
	res.append("<EventArray>");
	res.append(string(wres.begin(), wres.end()));
	res.append("</EventArray>");



	if (pwsMessage)
	{
		cout << endl << "End res : "<< res << endl;

		if (pwsMessage) {
			free(pwsMessage);
			pwsMessage = NULL;
		}
		return res;
	}


	//cout << "REs : " << endl << pwsMessage;




cleanup:

	if (hEvent)
		EvtClose(hEvent);

	if (hResults)
		EvtClose(hResults);

	if (hProviderMetadata)
		EvtClose(hProviderMetadata);


	return NULL;


}


LPWSTR GetMessageString(
	EVT_HANDLE hMetadata,
	EVT_HANDLE hEvent,
	EVT_FORMAT_MESSAGE_FLAGS FormatId,
	DWORD *dwBufferUsed
)
{
	LPWSTR pBuffer = NULL;
	DWORD dwBufferSize = 0;
	*dwBufferUsed = 0;
	
	DWORD status = 0;

	if (!EvtFormatMessage(hMetadata, hEvent, 0, 0, NULL, FormatId, dwBufferSize, pBuffer, dwBufferUsed))
	{
		status = GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == status)
		{
			if ((EvtFormatMessageKeyword == FormatId))
				pBuffer[*dwBufferUsed - 1] = L'\0';
			else
				dwBufferSize = *dwBufferUsed;

			pBuffer = (LPWSTR)malloc(dwBufferSize * sizeof(WCHAR));

			if (pBuffer)
			{
				EvtFormatMessage(hMetadata, hEvent, 0, 0, NULL, FormatId, dwBufferSize, pBuffer, dwBufferUsed);

				// Add the second null terminator character.
				if ((EvtFormatMessageKeyword == FormatId))
					pBuffer[*dwBufferUsed - 1] = L'\0';
			}
			else
			{
				wprintf(L"malloc failed\n");
			}
		}
		else if (ERROR_EVT_MESSAGE_NOT_FOUND == status || ERROR_EVT_MESSAGE_ID_NOT_FOUND == status)
			;
		else
		{
			wprintf(L"EvtFormatMessage failed with %u\n", status);
		}
	}

	return pBuffer;
}






void ListChannels() {
	EVT_HANDLE hChannels = NULL;
	LPWSTR pBuffer = NULL;
	LPWSTR pTemp = NULL;
	DWORD dwBufferSize = 0;
	DWORD dwBufferUsed = 0;
	DWORD status = ERROR_SUCCESS;

	// Get a handle to an enumerator that contains all the names of the 
	// channels registered on the computer.
	hChannels = EvtOpenChannelEnum(NULL, 0);

	if (NULL == hChannels)
	{
		wprintf(L"EvtOpenChannelEnum failed with %lu.\n", GetLastError());
		goto cleanup;
	}

	wprintf(L"List of Channels\n\n");

	// Enumerate through the list of channel names. If the buffer is not big
	// enough reallocate the buffer. To get the configuration information for
	// a channel, call the EvtOpenChannelConfig function.
	while (true)
	{
		if (!EvtNextChannelPath(hChannels, dwBufferSize, pBuffer, &dwBufferUsed))
		{
			status = GetLastError();

			if (ERROR_NO_MORE_ITEMS == status)
			{
				break;
			}
			else if (ERROR_INSUFFICIENT_BUFFER == status)
			{
				dwBufferSize = dwBufferUsed;
				pTemp = (LPWSTR)realloc(pBuffer, dwBufferSize * sizeof(WCHAR));
				if (pTemp)
				{
					pBuffer = pTemp;
					pTemp = NULL;
					EvtNextChannelPath(hChannels, dwBufferSize, pBuffer, &dwBufferUsed);
				}
				else
				{
					wprintf(L"realloc failed\n");
					status = ERROR_OUTOFMEMORY;
					goto cleanup;
				}
			}
			else
			{
				wprintf(L"EvtNextChannelPath failed with %lu.\n", status);
			}
		}

		wprintf(L"%s\n", pBuffer);
	}

cleanup:

	if (hChannels)
		EvtClose(hChannels);

	if (pBuffer)
		free(pBuffer);

}

DWORD PrintEvent(EVT_HANDLE hEvent)
{
	DWORD status = ERROR_SUCCESS;
	DWORD dwBufferSize = 0;
	DWORD dwBufferUsed = 0;
	DWORD dwPropertyCount = 0;
	LPWSTR pRenderedContent = NULL;

	if (!EvtRender(NULL, hEvent, EvtRenderEventXml, dwBufferSize, pRenderedContent, &dwBufferUsed, &dwPropertyCount))
	{
		if (ERROR_INSUFFICIENT_BUFFER == (status = GetLastError()))
		{
			dwBufferSize = dwBufferUsed;
			pRenderedContent = (LPWSTR)malloc(dwBufferSize);
			if (pRenderedContent)
			{

				EvtRender(
					NULL,
					hEvent,
					EvtRenderEventXml,
					dwBufferSize,
					pRenderedContent,
					&dwBufferUsed,
					&dwPropertyCount);
			}
			else
			{
				wprintf(L"malloc failed\n");
				status = ERROR_OUTOFMEMORY;
				goto cleanup;
			}
		}

		if (ERROR_SUCCESS != (status = GetLastError()))
		{
			wprintf(L"EvtRender failed with %d\n", status);
			goto cleanup;
		}
	}

	ZeroMemory(XMLDataCurrent, SIZE_DATA);
	lstrcpyW(XMLDataCurrent, pRenderedContent);

	wprintf(L"EvtRender data %s\n", XMLDataCurrent);

cleanup:

	if (pRenderedContent)
		free(pRenderedContent);


	return status;
}





// Enumerate all the events in the result set. 
DWORD PrintResults(EVT_HANDLE hResults)
{
	DWORD status = ERROR_SUCCESS;
	EVT_HANDLE hEvents[ARRAY_SIZE];
	DWORD dwReturned = 0;

	int count = 0;

	while (true)
	{
		// Get a block of events from the result set.
		if (!EvtNext(hResults, ARRAY_SIZE, hEvents, INFINITE, 0, &dwReturned))
		{
			if (ERROR_NO_MORE_ITEMS != (status = GetLastError()))
			{
				wprintf(L"EvtNext failed with %lu\n", status);
			}

			goto cleanup;
		}

		// For each event, call the PrintEvent function which renders the
		// event for display. PrintEvent is shown in RenderingEvents.

		for (DWORD i = 0; i < dwReturned; i++)
		{
			if (ERROR_SUCCESS == (status = PrintEvent(hEvents[i])))
			{
				EvtClose(hEvents[i]);
				hEvents[i] = NULL;
				count++;
			}
			else
			{
				goto cleanup;
			}
		}
	}

cleanup:

	for (DWORD i = 0; i < dwReturned; i++)
	{
		if (NULL != hEvents[i])
			EvtClose(hEvents[i]);
	}


	cout << "Total Count :  " << count;


	return status;
}


