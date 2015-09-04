import QtQuick 2.4
import QtQml 2.2
import QtQml.Models 2.2
import com.aragog.apprenticevideo 1.0
import '.'
import 'Utils.js' as Utils

Item
{
  id: player
  objectName: "player"

  signal exitFullScreen()
  signal toggleFullScreen()
  signal skipForward()
  signal skipBack()
  signal stepOneFrameForward()

  focus: true;

  Keys.onPressed:
  {
    if (playlist.view.visible)
    {
      playlist.handle_event_on_key_pressed(event);
    }
  }

  CanvasQuickFbo
  {
    id: renderer
    objectName: "renderer"

    anchors.fill: parent
    anchors.margins: 0

    // flip it right-side-up:
    transform:
    [
      Scale { yScale: -1; },
      Translate { y: renderer.height; }
    ]
  }

  MouseArea
  {
    anchors.fill: parent
    anchors.margins: 0

    onDoubleClicked: {
      toggleFullScreen();
      mouse.accepted = true;
    }
  }

  Playlist
  {
    id: playlist
    objectName: "playlist"
    visible: true

    anchors.margins: 0
    anchors.fill: parent
  }

  Timeline
  {
    id: timeline
    objectName: "timeline"
    visible: true

    anchors.margins: 0
    anchors.top: playlist.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.topMargin: -this.height
    height: (playlist.calc_title_height(24.0, playlist.width) * 3 / 2)
  }

}
