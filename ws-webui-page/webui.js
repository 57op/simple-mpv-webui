class WSApi {
  static _helper_connect(timeout = 5000) {
    return new Promise((resolve, reject) => {
      const ws = new WebSocket(`ws://${location.hostname}:${location.port}`, 'ws')

      setTimeout(() => reject('connection timeout'), timeout)
      ws.addEventListener('open', e => resolve(e.target))
    })
  }

  static _helper_send(ws, message, timeout = 5000) {
    return new Promise((resolve, reject) => {
      ws.send(JSON.stringify(message))

      setTimeout(() => reject('connection timeout'), timeout)

      ws.addEventListener('message', function listener (response) {
        ws.removeEventListener('message', listener)

        // just for testing, resolve with some random delay
        //setTimeout(() => resolve(JSON.parse(response.data)), (Math.random() * 250) | 0)
        resolve(JSON.parse(response.data))
      })

      ws.addEventListener('error', function listener (e) {
        ws.removeEventListener('error', listener)
        reject(e)
      })
    })
  }

  constructor (timeout = 5000) {
    this.ws = null
    this.timeout = timeout
  }

  get status () {
    return this.ws?.readyState ?? WebSocket.CLOSED
  }

  async connect () {
    this.ws = await WSApi._helper_connect(this.timeout)
    this.ws.addEventListener('close', () => {
      this.ws = null
    })
  }

  async send (message) {
    // attempt to connect
    if (this.status !== WebSocket.OPEN) {
      await this.connect()
    }

    return await WSApi._helper_send(this.ws, message, this.timeout)
  }

  async get_property (param) {
    const command = 'get_property'
    const property = await this.send({ command, param })

    if (property === false) {
      throw `failed to get property ${param}`
    }

    return property
  }

  async run_command (param) {
    const command = 'run_command'
    const result = await this.send({ command, param })

    if (result === false) {
      throw `failed to run command ${param}`
    }
  }
}

function main () {
  const app = new Vue({
    el: '#app',
    data: {
      mpv: null,

      overlay: false,

      metadata: null,
      tracklist: null,
      playlist: null,

      title: 'Title',

      subDelay: 0,
      audioDelay: 0,
      chapter: 1,
      chapters: 0,

      pause: false,
      position: 0,
      duration:  0,
      remaining: 0,
      loopFile: 'no',
      loopPlaylist: 'no',
      volume: 100,
      volumeMax: 0,
      fullscreen: false
    },

    computed: {
      connectionStatus () {
        return this.mpv?.status === WebSocket.OPEN
      },

      audioTracks () {
        return this.tracklist?.filter(tl => tl.type === 'audio').length ?? 0
      },

      currentAudioTrack () {
        return this.tracklist?.find(tl => tl.type === 'audio' && tl.selected === true)?.id ?? 0
      },

      subTracks () {
        return this.tracklist?.filter(tl => tl.type === 'sub').length ?? 0
      },

      currentSubTrack () {
        return this.tracklist?.find(tl => tl.type === 'sub' && tl.selected === true)?.id ?? 0
      }
    },

    methods: {
      async update () {
        try {
          this.metadata = await this.mpv.get_property('metadata')
          this.tracklist = await this.mpv.get_property('track-list')
          this.playlist = await this.mpv.get_property('playlist')

          this.title = await this.mpv.get_property('filename')

          this.subDelay = +(await this.mpv.get_property('sub-delay'))
          this.audioDelay = +(await this.mpv.get_property('audio-delay'))
          this.chapters = +(await this.mpv.get_property('chapters'))

          if (this.chapters > 0) {
            this.chapter = +(await this.mpv.get_property('chapter')) + 1
          }

          this.pause = await this.mpv.get_property('pause') === 'yes'
          this.position = +(await this.mpv.get_property('time-pos')) || 0
          this.duration = +(await this.mpv.get_property('duration'))
          this.remaining = +(await this.mpv.get_property('playtime-remaining'))
          this.loopFile = await this.mpv.get_property('loop-file')
          this.loopPlaylist = await this.mpv.get_property('loop-playlist')
          this.volume = +(await this.mpv.get_property('volume')) || 100
          this.volumeMax = +(await this.mpv.get_property('volume-max'))
          this.fullscreen = await this.mpv.get_property('fullscreen') === 'yes'
        } catch (e) {
          console.error(`UI Update failed: ${e}`)
        }
      },

      async command (cmd) {
        try {
          await this.mpv.run_command(cmd)
        } catch (e) {
          console.error(e)
        }
      },

      formatTime (seconds) {
        const date = new Date(0)

        if (!isNaN(seconds)) {
          date.setSeconds(seconds)
        }

        return date.toISOString().substr(11, 8)
      },

      entryName (entry) {
        let name

        if (entry.title) {
          name = entry.title
        } else {
          name = entry.filename.split('/')
          name = name[name.length - 1]
        }

        return name
      },

      async playlistLoopCycle () {
        const { loopFile, loopPlaylist } = this
        let lf
        let lp

        if (loopFile !== 'no') {
          // 1
          lf = 'no'
          lp = 'inf'
        } else if (loopPlaylist !== 'no') {
          // a
          lf = 'no'
          lp = 'no'
        } else {
          // no
          lf = 'inf'
          lp = 'no'
        }

        await this.command(`set loop-file ${lf}`)
        await this.command(`set loop-playlist ${lp}`)
      }
    },

    async created () {
      this.mpv = new WSApi(5000)
      await this.update()
      setInterval(async () => await this.update(), 1000)
    }
  })
}

document.addEventListener('DOMContentLoaded', main)
