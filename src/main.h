void GetCompilitionError( string AuthorID, string SubmitID, string Cookie );
void ParseStatus( string AuthorID, string SubmitID, string Response, string Cookie );
void UpdateStatus( string AuthorID, string SubmitID, string Cookie );
void InitSocket();
void Send( string Request );
string FormatRequest( string TimusID, string ProblemNum, string Language, string Code );
string HttpGet(string SendBuffer);