var DEBUG = false;
    loaded = false;
    metadata = {};
    subs = {};
    audios = {};
    blockPosSlider = false;
    blockVolSlider = false,
    intervalId = -1;

function wsConnect() {
  var ws = new WebSocket('ws://' + location.hostname + ':' + location.port, 'ws');

  ws.onopen = function() {
    send('status');

    if (intervalId > -1) {
      clearInterval(intervalId);
    }

    intervalId = setInterval(function() {
      send('status');
    }, 1000);
  };

  ws.onmessage = function(response) {
    var json = JSON.parse(response.data);
    handleStatusResponse(json);
  };

  ws.onerror = ws.onclose = function() {
    document.getElementById("title").innerHTML = "<h1><span class='error'>Couldn't connect to MPV!</span></h1>";
    document.getElementById("artist").innerHTML = "";
    document.getElementById("album").innerHTML = "";
    setPlayPause(true);
  };

  return ws;
}

var ws = wsConnect();

function send(command, param=null) {
  if (ws.readyState !== WebSocket.OPEN) {
    ws = wsConnect();
  }

  DEBUG && console.log('Sending command: ' + command + ' - param: ' + param);
  if ('mediaSession' in navigator) {
    audioLoad();
  }

  ws.send(JSON.stringify({ command, param }));
}

function togglePlaylist() {
  document.body.classList.toggle('noscroll');
  var el = document.getElementById("overlay");
  el.style.visibility = (el.style.visibility === "visible") ? "hidden" : "visible";
}

function createPlaylistTable(entry, position, pause, first) {
  function setActive(set) {
    if (set === true) {
      td_left.classList.add('active');
      td_2.classList.add('active');
    } else {
      td_left.classList.remove('active');
      td_2.classList.remove('active');
    }
  }

  function blink() {
     td_left.classList.add('click');
     td_2.classList.add('click');
     setTimeout(function(){ td_left.classList.remove('click');
     td_2.classList.remove('click');}, 100);
  }

  if (entry.title) {
    var title = entry.title;
  } else {
    var filename_array = entry.filename.split('/');
    title = filename_array[filename_array.length - 1];
  }

  var table = document.createElement('table');
  var tr = document.createElement('tr');
  var td_left = document.createElement('td');
  var td_2 = document.createElement('td');
  var td_right = document.createElement('td');
  table.className = 'playlist';
  tr.className = 'playlist';
  td_2.className = 'playlist';
  td_left.className = 'playlist';
  td_right.className = 'playlist';
  td_2.innerText = title;
  if (first === false) {
    var td_3 = document.createElement('td');
    td_3.innerHTML = '<i class="fas fa-arrow-up"></i>';
    td_3.className = 'playlist';
  }
  td_right.innerHTML = '<i class="fas fa-trash"></i>';

  if (entry.hasOwnProperty('playing')) {
    if (pause) {
      td_left.innerHTML = '<i class="fas fa-pause"></i>';
    } else {
      td_left.innerHTML = '<i class="fas fa-play"></i>';
    }

    td_left.classList.add('playing');
    td_left.classList.add('violet');
    td_2.classList.add('playing');
    td_2.classList.add('violet');
      first || td_3.classList.add('violet');
    td_right.classList.add('violet');

  } else {
    td_left.classList.add('gray');
    td_2.classList.add('gray');
    first || td_3.classList.add('gray');
    td_right.classList.add('gray');

    td_left.onclick = td_2.onclick = function(arg) {
        return function() {
            send("playlist_jump", arg);
            return false;
        }
    }(position);

    td_left.addEventListener("mouseover", function() {setActive(true)});
    td_left.addEventListener("mouseout", function() {setActive(false)});
    td_2.addEventListener("mouseover", function() {setActive(true)});
    td_2.addEventListener("mouseout", function() {setActive(false)});

    td_left.addEventListener("click", blink);
    td_2.addEventListener("click", blink);
  }

  if (first === false) {
    td_3.onclick = function (arg) {
      return function () {
        send("playlist_move_up", arg);
        return false;
      }
    }(position);
  }

  td_right.onclick = function(arg) {
      return function() {
          send("playlist_remove", arg);
          return false;
      }
  }(position);

  tr.appendChild(td_left);
  tr.appendChild(td_2);
  first || tr.appendChild(td_3);
  tr.appendChild(td_right);
  table.appendChild(tr);
  return table;
}

function populatePlaylist(json, pause) {
  var playlist = document.getElementById('playlist');
  playlist.innerHTML = "";

  var first = true;
  for(var i = 0; i < json.length; ++i) {
    playlist.appendChild(createPlaylistTable(json[i], i, pause, first));
    if (first === true) {
      first = false
    }
  }
}

window.onkeydown = function(e) {
  if (e.keyCode === 32 || e.key === ' ') {
    send('toggle_pause');
    return false;
  }
  if (e.keyCode === 39 || e.key === 'ArrowRight') {
    send('seek', '10');
    return false;
  }
  if (e.keyCode === 37 || e.key === 'ArrowLeft') {
    send('seek', '-10');
    return false;
  }
};

function format_time(seconds){
  var date = new Date(null);
  date.setSeconds(seconds);
  return date.toISOString().substr(11, 8);
}

function setFullscreenButton(fullscreen) {
    if (fullscreen) {
        var fullscreenText = 'Fullscreen off';
    } else {
        fullscreenText = 'Fullscreen on';
    }
    document.getElementById("fullscreenButton").innerText =
        fullscreenText;
}

function setTrackList(tracklist) {
  window.audios.selected = 0;
  window.audios.count = 0;
  window.subs.selected = 0;
  window.subs.count = 0;
  for (var i = 0; i < tracklist.length; i++){
    if (tracklist[i].type === 'audio') {
      window.audios.count++;
      if (tracklist[i].selected) {
          window.audios.selected = tracklist[i].id;
      }
    } else if (tracklist[i].type === 'sub') {
      window.subs.count++;
      if (tracklist[i].selected) {
          window.subs.selected = tracklist[i].id;
      }
    }
  }
  document.getElementById("nextSub").innerText = 'Next sub ' + window.subs.selected + '/' + window.subs.count;
  document.getElementById("nextAudio").innerText = 'Next audio ' + window.audios.selected + '/' + window.audios.count;
}

function setMetadata(metadata, playlist, filename) {
  // try to gather the track number
  if (metadata['track']) {
    var track = metadata['track'] + ' - ';
  } else {
    track = '';
  }

  // try to gather the playing playlist element
  for (var i = 0; i < playlist.length; i++){
    if (playlist[i].hasOwnProperty('playing')) {
       var pl_title = playlist[i].title;
    }
  }
  // set the title. Try values in this order:
  // 1. title set in playlist
  // 2. metadata['title']
  // 3. metadata['TITLE']
  // 4. filename
  if (pl_title) {
    window.metadata.title = track + pl_title;
  } else if (metadata['title']) {
    window.metadata.title = track + metadata['title'];
  } else if (metadata['TITLE']) {
    window.metadata.title = track + metadata['TITLE'];
  } else {
    window.metadata.title = track + filename;
  }

  // set the artist
  if (metadata['artist']) {
    window.metadata.artist = metadata['artist'];
  } else {
    window.metadata.artist = ''
  }

  // set the album
  if (metadata['album']) {
    window.metadata.album = metadata['album'];
  } else {
    window.metadata.album = ''
  }

  document.getElementById("title").innerHTML = window.metadata.title;
  document.getElementById("artist").innerHTML = window.metadata.artist;
  document.getElementById("album").innerHTML = window.metadata.album;
}

function setPosSlider(position, duration) {
  var slider = document.getElementById("mediaPosition");
  var pos = document.getElementById("position");
  slider.max = duration;
  if (!window.blockPosSlider) {
    slider.value = position;
  }
  pos.innerHTML = format_time(slider.value);
}

document.getElementById("mediaPosition").onchange = function() {
  var slider = document.getElementById("mediaPosition");
  send("set_position", slider.value);
  window.blockPosSlider = false;
};

document.getElementById("mediaPosition").oninput = function() {
  window.blockPosSlider = true;
  var slider = document.getElementById("mediaPosition");
  var pos = document.getElementById("position");
  pos.innerHTML = format_time(slider.value);
};

function setVolumeSlider(volume, volumeMax) {
  var slider = document.getElementById("mediaVolume");
  var vol = document.getElementById("volume");
  if (!window.blockVolSlider) {
    slider.value = volume;
    slider.max = volumeMax;
  }
  vol.innerHTML = slider.value + "%";
}

document.getElementById("mediaVolume").onchange = function() {
  var slider = document.getElementById("mediaVolume");
  send("set_volume", slider.value);
  window.blockVolSlider = false;
};

document.getElementById("mediaVolume").oninput = function() {
  window.blockVolSlider = true;
  var slider = document.getElementById("mediaVolume");
  var vol = document.getElementById("volume");
  vol.innerHTML = slider.value + "%";
};

function setPlayPause(value) {
  var playPause = document.getElementsByClassName('playPauseButton');

  // var playPause = document.getElementById("playPause");
  if (value) {
    [].slice.call(playPause).forEach(function (div) {
      div.innerHTML = '<i class="fas fa-play"></i>';
    });
    if ('mediaSession' in navigator) {
      audioPause();
    }
  } else {
    [].slice.call(playPause).forEach(function (div) {
      div.innerHTML = '<i class="fas fa-pause"></i>';
    });
    if ('mediaSession' in navigator) {
      audioPlay();
    }
  }
}

function playlist_loop_cycle() {
  var loopButton = document.getElementsByClassName('playlistLoopButton');
  if (loopButton.value === "no") {
    send("loop_file", "inf");
    send("loop_playlist", "no");
  } else if (loopButton.value === "1") {
    send("loop_file", "no");
    send("loop_playlist", "inf");
  } else if (loopButton.value === "a") {
    send("loop_file", "no");
    send("loop_playlist", "no");
  }
}

function setChapter(chapters, chapter) {
  var chapterButtons = document.getElementsByClassName('chapterButton');
  var chapterAddButtons = document.getElementsByClassName('chapterButtonAdd');
  var chapterSubButtons = document.getElementsByClassName('chapterButtonSub');
  var chapterTd = document.getElementById('chapter');
  var chapterInfo = document.getElementById('chapterInfo');
  if (chapters === 0) {
    [].slice.call(chapterButtons).forEach(function (div) {
      div.onclick = null;
      div.classList.add('disabled');
    });
    chapterTd.innerText = "0/0";
    chapterInfo.style.display = "none";
  } else {
    [].slice.call(chapterAddButtons).forEach(function (div) {
      div.onclick = function() {send('add_chapter', '1')};
      div.classList.remove('disabled');
    });
    [].slice.call(chapterSubButtons).forEach(function (div) {
      div.onclick = function() {send('add_chapter', '-1')};
      div.classList.remove('disabled');
    });
    chapterTd.innerText = chapter + 1 + "/" + chapters;
    chapterInfo.style.display = "";
  }

}

function setLoop(loopFile, loopPlaylist) {
  var loopButton = document.getElementsByClassName('playlistLoopButton');
  var html = '<i class="fas fa-redo-alt"></i>';
  var value = "";
  if (loopFile === "no") {
    if (loopPlaylist === "no") {
      html = '!<i class="fas fa-redo-alt"></i>';
      value = "no";
    } else {
      html = '<i class="fas fa-redo-alt"></i>Σ';
      value = "a";
    }
  } else {
    html = '<i class="fas fa-redo-alt"></i>1';
    value = "1";
  }

  [].slice.call(loopButton).forEach(function (div) {
      div.innerHTML = html;
    });

  loopButton.value = value;
}

function handleStatusResponse(json) {
  setMetadata(json['metadata'], json['playlist'], json['filename']);
  setTrackList(json['track-list']);
  document.getElementById("duration").innerHTML =
    '&nbsp;'+ format_time(json['duration']);
  document.getElementById("remaining").innerHTML =
    "-" + format_time(json['remaining']);
  document.getElementById("sub-delay").innerHTML =
    json['sub-delay'] + ' ms';
  document.getElementById("audio-delay").innerHTML =
    json['audio-delay'] + ' ms';
  setPlayPause(json['pause']);
  setPosSlider(json['position'], json['duration']);
  setVolumeSlider(json['volume'], json['volume-max']);
  setLoop(json["loop-file"], json["loop-playlist"]);
  setFullscreenButton(json['fullscreen']);
  setChapter(json['chapters'], json['chapter']);
  populatePlaylist(json['playlist'], json['pause']);
  if ('mediaSession' in navigator) {
    setupNotification();
  }
}

function audioLoad() {
  if (!window.loaded) {
    DEBUG && console.log('Loading dummy audio');
    document.getElementById("audio").load();
    window.loaded = true;
  }
}

function audioPlay() {
  var audio = document.getElementById("audio");
  if (audio.paused) {
    audio.play().then(function() {
      DEBUG && console.log('Playing dummy audio');
    });
  }
}

function audioPause() {
  var audio = document.getElementById("audio");
  if (!audio.paused) {
    DEBUG && console.log('Pausing dummy audio');
    audio.pause();
  }
}

function setupNotification() {
  if ('mediaSession' in navigator) {
    navigator.mediaSession.metadata = new MediaMetadata({
      title: window.metadata.title,
      artist: window.metadata.artist,
      album: window.metadata.album,
      artwork: [
        { src: '/favicons/android-chrome-192x192.png', sizes: '192x192', type: 'image/png' },
        { src: '/favicons/android-chrome-512x512.png', sizes: '512x512', type: 'image/png' }
      ]
    });

    navigator.mediaSession.setActionHandler('play', function() {send('play');});
    navigator.mediaSession.setActionHandler('pause', function() {send('pause');});
    navigator.mediaSession.setActionHandler('seekbackward', function() {send('seek', '-10');});
    navigator.mediaSession.setActionHandler('seekforward', function() {send('seek', '10');});
    navigator.mediaSession.setActionHandler('previoustrack', function() {send('playlist_prev');});
    navigator.mediaSession.setActionHandler('nexttrack', function() {send('playlist_next');});
  }
}

// prevent zoom-in on double-click
// https://stackoverflow.com/questions/37808180/disable-viewport-zooming-ios-10-safari/38573198#38573198
var lastTouchEnd = 0;
document.addEventListener('touchend', function (event) {
  var now = (new Date()).getTime();
  if (now - lastTouchEnd <= 300) {
    event.preventDefault();
  }
  lastTouchEnd = now;
}, false);
