/* Manual HTTPS update check. No background polling or executable downloads. */
static int compareRelease(const char *tag)
{
    unsigned int parts[3] = {0};
    const char *p = tag;
    if (*p == 'v') ++p;
    for (int i = 0; i < 3; ++i) {
        if (!isdigit((unsigned char)*p)) return 0;
        while (isdigit((unsigned char)*p)) {
            parts[i] = parts[i] * 10 + (*p++ - '0');
            if (parts[i] > 9999) return 0;
        }
        if (!*p) break;
        if (*p++ != '.' || i == 2) return 0;
    }
    const unsigned int current[3] = {2, 2, 0};
    for (int i = 0; i < 3; ++i)
        if (parts[i] != current[i]) return parts[i] > current[i] ? 2 : 1;
    return 1;
}
static BOOL releaseTag(const char *json, char tag[48])
{
    /* Match only a top-level JSON key, ignoring strings and nested objects. */
    int depth = 0;
    for (const char *p = json; *p; ++p) {
        if (*p == '{' || *p == '[') ++depth;
        else if (*p == '}' || *p == ']') --depth;
        else if (*p == '"') {
            const char *start = ++p;
            while (*p && *p != '"') {
                if (*p == '\\' && p[1]) ++p;
                ++p;
            }
            if (!*p) return FALSE;
            if (depth != 1 || p - start != 8 || memcmp(start, "tag_name", 8)) continue;
            const char *value = p + 1;
            while (isspace((unsigned char)*value)) ++value;
            if (*value++ != ':') continue;
            while (isspace((unsigned char)*value)) ++value;
            if (*value++ != '"') return FALSE;
            int n = 0;
            while (*value && *value != '"') {
                if (n == 47 || (!isdigit((unsigned char)*value) && *value != '.' && *value != 'v')) return FALSE;
                tag[n++] = *value++;
            }
            tag[n] = 0;
            return *value == '"' && compareRelease(tag) != 0;
        }
    }
    return FALSE;
}
static int fetchUpdate(void)
{
    int result = 0;
    HINTERNET session = WinHttpOpen(L"CopilotKeyBinding/2.2", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return 0;
    WinHttpSetTimeouts(session, 5000, 5000, 5000, 10000);
    HINTERNET connection = WinHttpConnect(session, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET request = connection ? WinHttpOpenRequest(connection, L"GET",
        L"/repos/Milorgs/CopilotKeyBinding/releases/latest", NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : NULL;
    char *body = NULL;
    if (request && WinHttpSendRequest(request, L"Accept: application/vnd.github+json\r\n", (DWORD)-1,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(request, NULL)) {
        DWORD status = 0, size = sizeof(status);
        if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX) && status == 200) {
            body = (char *)malloc(131073);
            if (body) {
                DWORD total = 0, count;
                BOOL complete = FALSE;
                while (total < 131072) {
                    if (!WinHttpReadData(request, body + total, 131072 - total, &count)) break;
                    if (!count) { complete = TRUE; break; }
                    total += count;
                }
                body[total] = 0;
                char tag[48];
                if (complete && releaseTag(body, tag)) result = compareRelease(tag);
            }
        }
    }
    free(body);
    if (request) WinHttpCloseHandle(request);
    if (connection) WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return result;
}
static DWORD WINAPI updateThread(void *context)
{
    int result = fetchUpdate();
    PostMessageW((HWND)context, UPDATE_COMPLETE, result, 0);
    return 0;
}
