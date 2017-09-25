#include <iostream>
#include <cstdlib>
#include <libnavajo/libnavajo.hh>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <jsoncpp/json/json.h>
#include <mutex>
#include <queue>
#include <vlc/libvlc.h>
#include <vlc/libvlc_renderer_discoverer.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>
#include <vlc/libvlc_events.h>
#include <vlc/libvlc_media_list.h>
#include <thread>
#include <fcntl.h>
#include <cstdio>
#include <unistd.h>
using namespace std;
cURLpp::Easy *easyhandle;
class MusicQueue {
public:
    void QueueSong(string url) {
        std::lock_guard<std::mutex> lock(a);
        s.push(url);
    }
    string GetLatestSong() {
        string r = s.front();
        s.pop();
        return r;
    }
    bool isEmpty() {
        return s.empty();
    }

private:
    std::mutex a;
    std::queue<string> s;
};

MusicQueue que;
timeval pstart;

//1620zdncfxbb08bbn5hwo7is9d0l9n
//f851e09664d834c7c28e4a39fce0bc65d

std::string str_tolower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); } // correct
    );
    return s;
}

class TwitchPage : public DynamicPage {
    bool getPage(HttpRequest *request, HttpResponse *response) {
        if(std::string(request->getUrl()) == "auth_res") {
            vector<string> c = request->getCookiesNames();
            for(string n : c) {
                if(n == "authcode") {
                    return fromString("<html><body><a href=\"http://104.56.93.33:88/mus\">go here now</a></body></html>", response);
                }
            }
            vector<string> a = request->getParameterNames();
            if(a.size() == 0) {
                return fromString("<html><body><a href=\"https://api.twitch.tv/kraken/oauth2/authorize?client_id=1620zdncfxbb08bbn5hwo7is9d0l9n&redirect_uri=http://104.56.93.33:88/auth_res&response_type=code&scope=&state=ffffffffffffff\">login with twtich</a></body></html>", response);
            }
            for(string name : a) {
                if(name == "code"){
                    response->addCookie("authcode", request->getParameter("code"));
                    return fromString("<html><body><a href=\"http://104.56.93.33:88/mus\">go here now</a></body></html>", response);
                }
            }
            return fromString("you fucked it", response);
        } else if(std::string(request->getUrl()) == "mus") {
            return fromString("<html><body><form action=\"post\" method=\"post\"> Url: <input type=\"text\" name=\"url\"><br><input type=\"submit\" value=\"Submit\"></form></body></html>", response);
        } else if(std::string(request->getUrl()) == "post") {
            //response->forwardTo("104.56.93.33:88/mus");
            std::vector<string> p = request->getParameterNames();
            if(request->hasParameter("url")) {
                timeval pnow;
                gettimeofday(&pnow, NULL);
                if((pnow.tv_sec - pstart.tv_sec) > 60) {
                    m = map<string, uint64_t>();
                    pstart = pnow;
                }
                std::cout << request->getPayload().data() << std::endl;
                if(str_tolower(request->getParameter("url").substr(0, 4))  == "file") {
                    return fromString("<html><body><a href=\"http://104.56.93.33:88/mus\">no</a></body></html>", response);
                }
                if(str_tolower(request->getParameter("url").substr(0, 6))  == "screen") {
                    return fromString("<html><body><a href=\"http://104.56.93.33:88/mus\">no</a></body></html>", response);
                }
                m[request->getPeerIpAddress().str()]++;
                if(m[request->getPeerIpAddress().str()] > 1) {
                    return fromString("<html><body><a href=\"http://104.56.93.33:88/mus\">hold up</a></body></html>", response);

                }
                que.QueueSong(request->getParameter("url"));
                std::cout << request->getParameter("url") << std::endl;
            }
            return fromString("<html><body><a href=\"http://104.56.93.33:88/mus\">go here now</a></body></html>", response);
        }
    }
private:
    std::map<string, uint64_t> m;
};

bool stopped = false;
void hack(const struct libvlc_event_t *p_event, void *p_data) {
    stopped = true;
}


int main(int argc, char *argv[])
{
    int flags;
    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(STDIN_FILENO, F_SETFL, flags);
    gettimeofday(&pstart, NULL);
    cURLpp::initialize(CURL_GLOBAL_ALL);
    easyhandle = new cURLpp::Easy;
    //easyhandle->
    WebServer w;
    w.setServerPort(88);
    TwitchPage twitch;
    DynamicRepository repo;
    repo.add("/auth_res", &twitch);
    repo.add("/mus", &twitch);
    repo.add("/post", &twitch);
    w.addRepository(&repo);
    w.startService();
    libvlc_instance_t *vlc_inst;
    libvlc_media_player_t *media_player;
    libvlc_event_manager_t *event_manager;
    vlc_inst = libvlc_new(0, NULL);
    media_player = libvlc_media_player_new(vlc_inst);
    event_manager = libvlc_media_player_event_manager(media_player);
    libvlc_event_attach(event_manager, libvlc_MediaPlayerStopped, &hack,  NULL);
    libvlc_event_attach(event_manager, libvlc_MediaPlayerEncounteredError, &hack,  NULL);
    while(true) {
        if(que.isEmpty()) {
            continue;
        }
        stopped = false;
        libvlc_media_t *media;
        media = libvlc_media_new_location(vlc_inst, que.GetLatestSong().c_str());
        libvlc_media_player_set_media(media_player, media);
        libvlc_media_player_play(media_player);
        libvlc_media_player_set_xwindow(media_player, 470327403);
        std::cout << libvlc_media_player_get_length(media_player) << std::endl;
        while(!stopped) {

        }
        libvlc_media_list_t *l = libvlc_media_subitems(media);
        if((l != NULL) && (libvlc_media_list_count(l) > 0)) {
            libvlc_media_t * m = libvlc_media_list_item_at_index(l, 0);
            libvlc_media_player_set_media(media_player, m);
            stopped = false;
            libvlc_media_player_play(media_player);
            while(!stopped) {
                if(fgetc(stdin) == 's') {
                    libvlc_media_player_stop(media_player);
                    break;
                }
            }
        }
    }
    return 0;
}
