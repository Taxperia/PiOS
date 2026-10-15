// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

Item {
    id: root
    property string name: "apps"
    property color color: "white"
    property real strokeWidth: 1.8

    implicitWidth: 22
    implicitHeight: 22

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true

        function line(ctx, points) {
            ctx.beginPath()
            ctx.moveTo(points[0][0], points[0][1])
            for (let i = 1; i < points.length; ++i)
                ctx.lineTo(points[i][0], points[i][1])
            ctx.stroke()
        }

        function roundedRect(ctx, x, y, w, h, r, fill) {
            ctx.beginPath()
            ctx.roundedRect(x, y, w, h, r, r)
            if (fill) ctx.fill(); else ctx.stroke()
        }

        onPaint: {
            const ctx = getContext("2d")
            const w = width
            const h = height
            const sx = w / 24
            const sy = h / 24
            ctx.reset()
            ctx.scale(sx, sy)
            ctx.strokeStyle = root.color
            ctx.fillStyle = root.color
            ctx.lineWidth = root.strokeWidth
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            if (root.name === "home") {
                line(ctx, [[4,11],[12,4],[20,11]])
                ctx.beginPath(); ctx.moveTo(6.5,10); ctx.lineTo(6.5,20); ctx.lineTo(17.5,20); ctx.lineTo(17.5,10); ctx.stroke()
                roundedRect(ctx, 10,14,4,6,1,false)
            } else if (root.name === "terminal") {
                line(ctx, [[5,7],[10,12],[5,17]])
                line(ctx, [[12,17],[19,17]])
            } else if (root.name === "folder") {
                ctx.beginPath(); ctx.moveTo(3,7); ctx.lineTo(9,7); ctx.lineTo(11,9); ctx.lineTo(21,9); ctx.lineTo(20,19); ctx.lineTo(4,19); ctx.closePath(); ctx.fill()
                roundedRect(ctx,3,6,7,4,1,true)
            } else if (root.name === "new-folder") {
                ctx.beginPath(); ctx.moveTo(3,7); ctx.lineTo(9,7); ctx.lineTo(11,9); ctx.lineTo(21,9); ctx.lineTo(20,19); ctx.lineTo(4,19); ctx.closePath(); ctx.stroke()
                roundedRect(ctx,3,6,7,4,1,false); line(ctx,[[12,12],[12,17]]); line(ctx,[[9.5,14.5],[14.5,14.5]])
            } else if (root.name === "file") {
                ctx.beginPath(); ctx.moveTo(6,3); ctx.lineTo(14,3); ctx.lineTo(19,8); ctx.lineTo(19,21); ctx.lineTo(6,21); ctx.closePath(); ctx.stroke()
                line(ctx,[[14,3],[14,8],[19,8]])
            } else if (root.name === "clipboard") {
                roundedRect(ctx,5,6,14,15,2,false); roundedRect(ctx,9,3,6,5,2,false)
            } else if (root.name === "globe") {
                ctx.beginPath(); ctx.arc(12,12,8.5,0,Math.PI*2); ctx.stroke()
                ctx.beginPath(); ctx.ellipse(12,12,4,8.5,0,0,Math.PI*2); ctx.stroke()
                line(ctx, [[4,12],[20,12]])
            } else if (root.name === "store") {
                roundedRect(ctx,4,8,16,12,2,true)
                ctx.strokeStyle = root.color; ctx.fillStyle = "transparent"
                ctx.beginPath(); ctx.arc(12,8,4,Math.PI,Math.PI*2); ctx.stroke()
                ctx.fillStyle = "#EB4D62"; roundedRect(ctx,7,11,10,6,1,true)
            } else if (root.name === "play") {
                roundedRect(ctx,3,3,18,18,4,true)
                ctx.fillStyle = "white"; ctx.beginPath(); ctx.moveTo(10,8); ctx.lineTo(16,12); ctx.lineTo(10,16); ctx.closePath(); ctx.fill()
            } else if (root.name === "chip") {
                roundedRect(ctx,6,6,12,12,2,false); roundedRect(ctx,9,9,6,6,1,false)
                for (let i=0;i<4;i++) { line(ctx, [[8+i*3,3],[8+i*3,6]]); line(ctx, [[8+i*3,18],[8+i*3,21]]); line(ctx, [[3,8+i*3],[6,8+i*3]]); line(ctx, [[18,8+i*3],[21,8+i*3]]) }
            } else if (root.name === "apps" || root.name === "grid") {
                for (let y=0;y<3;y++) for (let x=0;x<3;x++) roundedRect(ctx,5+x*6,5+y*6,3,3,0.8,true)
            } else if (root.name === "search") {
                ctx.beginPath(); ctx.arc(10.5,10.5,5.5,0,Math.PI*2); ctx.stroke(); line(ctx, [[15,15],[20,20]])
            } else if (root.name === "picture") {
                roundedRect(ctx,3,4,18,16,2,false); ctx.beginPath(); ctx.arc(8,9,2,0,Math.PI*2); ctx.stroke()
                line(ctx,[[5,18],[10,13],[13,16],[16,12],[20,17]])
            } else if (root.name === "display") {
                roundedRect(ctx,3,4,18,13,2,false); line(ctx,[[12,17],[12,21]]); line(ctx,[[8,21],[16,21]])
            } else if (root.name === "sort") {
                ctx.beginPath(); ctx.arc(5,6,1.5,0,Math.PI*2); ctx.fill(); line(ctx,[[9,6],[20,6]])
                ctx.beginPath(); ctx.arc(5,12,1.5,0,Math.PI*2); ctx.fill(); line(ctx,[[9,12],[17,12]])
                ctx.beginPath(); ctx.arc(5,18,1.5,0,Math.PI*2); ctx.fill(); line(ctx,[[9,18],[20,18]])
            } else if (root.name === "refresh") {
                ctx.beginPath(); ctx.arc(12,12,8,0.25,Math.PI*1.55); ctx.stroke(); line(ctx,[[4,6],[4,12],[9,9]])
                ctx.beginPath(); ctx.arc(12,12,8,Math.PI*1.25,Math.PI*2.05); ctx.stroke(); line(ctx,[[20,18],[20,12],[15,15]])
            } else if (root.name === "personalize") {
                ctx.beginPath(); ctx.moveTo(5,19); ctx.quadraticCurveTo(7,11,18,5); ctx.quadraticCurveTo(17,15,9,18); ctx.closePath(); ctx.stroke()
                line(ctx,[[6,20],[13,12]])
            } else if (root.name === "wifi") {
                ctx.beginPath(); ctx.arc(12,18,1.5,0,Math.PI*2); ctx.fill()
                ctx.beginPath(); ctx.arc(12,18,6,Math.PI*1.2,Math.PI*1.8); ctx.stroke()
                ctx.beginPath(); ctx.arc(12,18,10,Math.PI*1.2,Math.PI*1.8); ctx.stroke()
            } else if (root.name === "bluetooth") {
                line(ctx, [[12,3],[17,8],[8,17],[12,21],[12,3],[17,8],[7,15]])
            } else if (root.name === "moon") {
                ctx.beginPath(); ctx.arc(11,11,8,0,Math.PI*2); ctx.fill()
                ctx.globalCompositeOperation = "destination-out"; ctx.beginPath(); ctx.arc(15,8,7,0,Math.PI*2); ctx.fill(); ctx.globalCompositeOperation = "source-over"
            } else if (root.name === "bolt") {
                ctx.beginPath(); ctx.moveTo(13,2); ctx.lineTo(5,14); ctx.lineTo(11,14); ctx.lineTo(9,22); ctx.lineTo(19,10); ctx.lineTo(13,10); ctx.closePath(); ctx.fill()
            } else if (root.name === "lock") {
                roundedRect(ctx,5,10,14,11,2,false); ctx.beginPath(); ctx.arc(12,10,4,Math.PI,Math.PI*2); ctx.stroke(); roundedRect(ctx,11,14,2,4,1,true)
            } else if (root.name === "restart") {
                ctx.beginPath(); ctx.arc(12,13,7,Math.PI*0.15,Math.PI*1.85); ctx.stroke(); line(ctx,[[5,7],[5,13],[10,10]])
            } else if (root.name === "power") {
                line(ctx,[[12,3],[12,12]]); ctx.beginPath(); ctx.arc(12,13,8,Math.PI*1.7,Math.PI*1.3); ctx.stroke()
            } else if (root.name === "volume") {
                ctx.beginPath(); ctx.moveTo(4,10); ctx.lineTo(8,10); ctx.lineTo(13,6); ctx.lineTo(13,18); ctx.lineTo(8,14); ctx.lineTo(4,14); ctx.closePath(); ctx.fill()
                ctx.beginPath(); ctx.arc(12,12,6,-0.7,0.7); ctx.stroke(); ctx.beginPath(); ctx.arc(12,12,9,-0.7,0.7); ctx.stroke()
            } else if (root.name === "sun") {
                ctx.beginPath(); ctx.arc(12,12,3.5,0,Math.PI*2); ctx.stroke()
                for (let i=0;i<8;i++) { const a=i*Math.PI/4; line(ctx,[[12+6*Math.cos(a),12+6*Math.sin(a)],[12+9*Math.cos(a),12+9*Math.sin(a)]]) }
            } else if (root.name === "bell") {
                ctx.beginPath(); ctx.moveTo(6,16); ctx.quadraticCurveTo(8,14,8,10); ctx.arc(12,10,4,Math.PI,0); ctx.quadraticCurveTo(16,14,18,16); ctx.closePath(); ctx.stroke(); line(ctx,[[10,19],[14,19]])
            } else if (root.name === "menu") {
                for (let i=0;i<4;i++) { const a=i*Math.PI/2; ctx.beginPath(); ctx.arc(12+5*Math.cos(a),12+5*Math.sin(a),2.6,0,Math.PI*2); ctx.fill() }
                ctx.beginPath(); ctx.arc(12,12,2.8,0,Math.PI*2); ctx.fill()
            } else if (root.name === "temperature") {
                roundedRect(ctx,10,3,4,13,2,false); ctx.beginPath(); ctx.arc(12,18,4,0,Math.PI*2); ctx.fill(); roundedRect(ctx,11,7,2,10,1,true)
            } else if (root.name === "settings") {
                ctx.beginPath(); ctx.arc(12,12,4,0,Math.PI*2); ctx.stroke()
                ctx.beginPath(); ctx.arc(12,12,8,0,Math.PI*2); ctx.stroke()
                for (let i=0;i<8;i++) { const a=i*Math.PI/4; line(ctx,[[12+8*Math.cos(a),12+8*Math.sin(a)],[12+10*Math.cos(a),12+10*Math.sin(a)]]) }
            } else if (root.name === "board") {
                roundedRect(ctx,3,5,18,14,2,true); ctx.fillStyle="#12271E"; roundedRect(ctx,7,8,7,7,1,true); ctx.fillStyle=root.color; roundedRect(ctx,16,7,3,3,0.5,true); roundedRect(ctx,16,12,3,2,0.5,true)
            } else {
                ctx.beginPath(); ctx.arc(12,12,7,0,Math.PI*2); ctx.stroke()
            }
        }

        Connections {
            target: root
            function onNameChanged() { canvas.requestPaint() }
            function onColorChanged() { canvas.requestPaint() }
            function onStrokeWidthChanged() { canvas.requestPaint() }
        }
    }
}
