#include <unistd.h>
#include "server/webserver.h"

int main() {
    // �ػ����� ��̨���� 
    WebServer server(
        1316, 3, 60000, false,             /* �˿� ETģʽ timeoutMs �����˳�  */
        3306, "root", "123456", "webserver", /* Mysql���� */
        12, 6, true, 1, 1024);             /* ���ӳ����� �̳߳����� ��־���� ��־�ȼ� ��־�첽�������� */
    server.Start();
} 

