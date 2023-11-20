import {connect, } from "mqtt"
import { useState } from "react";
import axios from "axios"

function App() {

  const [pubMessage, setPubMessage] = useState()

  const handlePublish = () => {
    const client = connect("ws://192.168.0.197:8080");
    client.publish("/Hello", pubMessage);
    client.subscribe("/topic/help");
    client.on("message", (topic, message) => {
      console.log(message.toString())
    })
  }

  return (
    <>
      <input type="file" onChange={async (event) => {
          const form = new FormData()
          form.set("file", event.target.files[0])
          const res = await axios.post('http://localhost:4000/files',form)
          console.log(res)
      }} />
      <input placeholder="Enter Message to publish" onChange={(e) => setPubMessage(e.target.value)}/>
      <button onClick={handlePublish}>Publish</button>
      
    </>
  )
}

export default App
