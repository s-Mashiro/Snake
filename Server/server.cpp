// #include <mysql/errmsg.h>
#include <mysql/mysql.h>

#include <boost/asio.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;
using namespace std;

#define ER_DUP_ENTRY 1062

static constexpr auto DB_HOST = "127.0.0.1";
static constexpr auto DB_USER = "";
static constexpr auto DB_PASS = "";
static constexpr auto DB_NAME = "";
static constexpr unsigned DB_PORT = 3306;
static constexpr unsigned Server_PORT = 49157;

static mutex io_mtx;

MYSQL* get_db_connection() {
    MYSQL* conn = mysql_init(nullptr);
    if (!conn) return nullptr;

    unsigned timeout = 5;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, nullptr, 0)) {
        std::cerr << "Connect error: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return nullptr;
    }
    return conn;
}

void write_response(tcp::socket& sock, uint8_t code) {
    boost::asio::write(sock, boost::asio::buffer(&code, 1));
}

void handle_register(tcp::socket sock) {
    try {
        uint8_t ulen, plen, elen;
        boost::asio::read(sock, boost::asio::buffer(&ulen, 1));
        vector<char> ubuf(ulen);
        boost::asio::read(sock, boost::asio::buffer(ubuf));

        boost::asio::read(sock, boost::asio::buffer(&plen, 1));
        vector<char> pbuf(plen);
        boost::asio::read(sock, boost::asio::buffer(pbuf));

        boost::asio::read(sock, boost::asio::buffer(&elen, 1));
        vector<char> ebuf(elen);
        boost::asio::read(sock, boost::asio::buffer(ebuf));

        string username(ubuf.begin(), ubuf.end());
        string password(pbuf.begin(), pbuf.end());
        string email(ebuf.begin(), ebuf.end());

        {
            lock_guard<mutex> lg(io_mtx);
            cout << "[REGISTER] " << username << " / " << password << " / " << email << "\n";
        }

        uint8_t resp = 0x03;
        if (auto* conn = get_db_connection()) {
            string eu(username.size() * 2 + 1, '\0');
            string ep(password.size() * 2 + 1, '\0');
            string ee(email.size() * 2 + 1, '\0');
            auto un = mysql_real_escape_string(conn, &eu[0], username.c_str(), username.size());
            auto pn = mysql_real_escape_string(conn, &ep[0], password.c_str(), password.size());
            auto en = mysql_real_escape_string(conn, &ee[0], email.c_str(), email.size());
            eu.resize(un);
            ep.resize(pn);
            ee.resize(en);

            string q = "INSERT INTO `user`(username,password,email) VALUES('" + eu + "','" + ep +
                       "','" + ee + "')";
            if (mysql_query(conn, q.c_str()) == 0) {
                resp = 0x01;
            } else if (mysql_errno(conn) == ER_DUP_ENTRY) {
                resp = 0x02;
            }
            mysql_close(conn);
        }

        write_response(sock, resp);
    } catch (const exception& e) {
        lock_guard<mutex> lg(io_mtx);
        cerr << "handle_register exception: " << e.what() << "\n";
    }
}

void handle_login(tcp::socket sock) {
    try {
        uint8_t ulen, plen;
        boost::asio::read(sock, boost::asio::buffer(&ulen, 1));
        vector<char> ubuf(ulen);
        boost::asio::read(sock, boost::asio::buffer(ubuf));

        boost::asio::read(sock, boost::asio::buffer(&plen, 1));
        vector<char> pbuf(plen);
        boost::asio::read(sock, boost::asio::buffer(pbuf));

        string username(ubuf.begin(), ubuf.end());
        string password(pbuf.begin(), pbuf.end());

        {
            lock_guard<mutex> lg(io_mtx);
            cout << "[LOGIN]  " << username << " / " << password << "\n";
        }

        uint8_t resp = 0x02;
        if (auto* conn = get_db_connection()) {
            string eu(username.size() * 2 + 1, '\0');
            auto un = mysql_real_escape_string(conn, &eu[0], username.c_str(), username.size());
            eu.resize(un);

            string q = "SELECT password FROM user WHERE username='" + eu + "' LIMIT 1";
            if (mysql_query(conn, q.c_str()) == 0) {
                if (auto* res = mysql_store_result(conn)) {
                    if (mysql_num_rows(res) == 1) {
                        auto row = mysql_fetch_row(res);
                        if (row[0] && password == row[0]) {
                            resp = 0x01;
                        }
                    }
                    mysql_free_result(res);
                }
            }
            mysql_close(conn);
        }

        write_response(sock, resp);
    } catch (const exception& e) {
        lock_guard<mutex> lg(io_mtx);
        cerr << "handle_login exception: " << e.what() << "\n";
    }
}

void handle_record(tcp::socket sock) {
    try {
        uint8_t ulen, slen;
        boost::asio::read(sock, boost::asio::buffer(&ulen, 1));
        vector<char> ubuf(ulen);
        boost::asio::read(sock, boost::asio::buffer(ubuf));

        boost::asio::read(sock, boost::asio::buffer(&slen, 1));
        vector<char> sbuf(slen);
        boost::asio::read(sock, boost::asio::buffer(sbuf));

        string username(ubuf.begin(), ubuf.end());
        int score = stoi(string(sbuf.begin(), sbuf.end()));

        {
            lock_guard<mutex> lg(io_mtx);
            cout << "[RECORD] " << username << " -> " << score << "\n";
        }

        uint8_t resp = 0x03;
        if (auto* conn = get_db_connection()) {
            string eu(username.size() * 2 + 1, '\0');
            auto un = mysql_real_escape_string(conn, &eu[0], username.c_str(), username.size());
            eu.resize(un);

            string q1 = "SELECT id FROM user WHERE username='" + eu + "' LIMIT 1";
            if (mysql_query(conn, q1.c_str()) == 0) {
                if (auto* res1 = mysql_store_result(conn)) {
                    if (mysql_num_rows(res1) == 1) {
                        int uid = stoi(mysql_fetch_row(res1)[0]);
                        mysql_free_result(res1);

                        string q2 = "INSERT INTO record(user_id,score) VALUES(" + to_string(uid) +
                                    "," + to_string(score) + ")";
                        if (mysql_query(conn, q2.c_str()) == 0) {
                            resp = 0x01;
                        }
                    } else {
                        mysql_free_result(res1);
                    }
                }
            }
            mysql_close(conn);
        }

        write_response(sock, resp);
    } catch (const exception& e) {
        lock_guard<mutex> lg(io_mtx);
        cerr << "handle_record exception: " << e.what() << "\n";
    }
}

void handle_top10(tcp::socket sock) {
    try {
        lock_guard<mutex> lg(io_mtx);
        cout << "[TOP10] request\n";

        if (auto* conn = get_db_connection()) {
            const char* q =
                "SELECT u.username, r.score, r.completion_time "
                "FROM record r "
                "JOIN ("
                "    SELECT user_id, MAX(score) AS max_score "
                "    FROM record "
                "    GROUP BY user_id"
                ") m ON r.user_id = m.user_id AND r.score = m.max_score "
                "JOIN `user` u ON u.id = r.user_id "
                "ORDER BY r.score DESC "
                "LIMIT 10";

            if (mysql_query(conn, q) == 0) {
                if (auto* res = mysql_store_result(conn)) {
                    std::ostringstream oss;
                    MYSQL_ROW row;
                    while ((row = mysql_fetch_row(res)) != nullptr) {
                        const char* username = row[0] ? row[0] : "";
                        const char* score = row[1] ? row[1] : "0";
                        const char* ctime = row[2] ? row[2] : "";
                        oss << username << '\t' << score << '\t' << ctime << '\n';
                    }
                    mysql_free_result(res);

                    std::string out = oss.str();
                    uint8_t resp = 0x01;
                    boost::asio::write(sock, boost::asio::buffer(&resp, 1));
                    boost::asio::write(sock, boost::asio::buffer(out));
                }
            }
            mysql_close(conn);
        }
    } catch (const exception& e) {
        lock_guard<mutex> lg(io_mtx);
        cerr << "handle_top10 exception: " << e.what() << "\n";
    }
}

void handle_unknown(tcp::socket sock, uint8_t header) {
    lock_guard<mutex> lg(io_mtx);
    cout << "[UNKNOWN HEADER] 0x" << hex << int(header) << dec << "\n";
}

int main() {
    try {
        boost::asio::io_context io_ctx;
        tcp::acceptor acceptor(io_ctx, {tcp::v4(), Server_PORT});
        cout << "Server listening on port " << Server_PORT << "...\n";

        while (true) {
            tcp::socket sock(io_ctx);
            acceptor.accept(sock);

            uint8_t header = 0;
            boost::asio::read(sock, boost::asio::buffer(&header, 1));

            switch (header) {
                case 0x01:
                    thread(handle_register, std::move(sock)).detach();
                    break;
                case 0x02:
                    thread(handle_login, std::move(sock)).detach();
                    break;
                case 0x03:
                    thread(handle_record, std::move(sock)).detach();
                    break;
                case 0x04:
                    thread(handle_top10, std::move(sock)).detach();
                    break;
                default:
                    thread(handle_unknown, std::move(sock), header).detach();
                    break;
            }
        }
    } catch (const exception& e) {
        cerr << "Fatal server error: " << e.what() << "\n";
    }
    return 0;
}
