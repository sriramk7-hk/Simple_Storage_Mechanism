import express, { response } from "express"
import mqtt from "mqtt"
import { createHelia } from "helia"
import { strings } from "@helia/strings"
import mongoose from "mongoose"
import ipfsdata from "./ipfsdata.js"
import multer from "multer"
import cors from "cors"
import fs from "fs"
import path from "path"
import pinataSDK from "@pinata/sdk"
import 'dotenv/config'

const connectAndSubscribe = () => {
    let client = mqtt.connect("ws://192.168.0.197:8080")
    client.on('connect', () => {
        console.log(`Successfully Connected to MQTT`)
        client.subscribe('/topic/help', (err) => {
            if(err){
                console.log(err)
            }
        })
    })
    client.on('message', (topic, payload, packet) => {
        console.log(payload.toString())
    })
}

const app = express()
const helia = await createHelia()
const saveString = strings(helia)

const pinata = new pinataSDK(
    process.env.PINATA_API_KEY,
    process.env.PINATA_API_SECRET
);

const storage = multer.diskStorage({
  destination: (req, file, callback) => {
      callback(null, path.join("./fileUploads/"))
  },
  filename: (req, file, callback) => {
      const fileName = "file"
      callback(null, fileName)
  }
})

const upload = multer({storage}).fields([{name: "file"}]);

mongoose.connect('mongodb://127.0.0.1:27017')
    .then(() => console.log("Successfully connected to database"))
    .catch((error) => console.log(`===> Error: ${error}`));

app.use(cors({credentials: true, origin: "*"}))
app.use(express.json());
app.use(express.urlencoded({ extended: true }))

app.get('/', async (req, res) => {
    try {
        const ipfsAddresses = await ipfsdata.find({})
        res.send({status: 400, msg: "Success", data: ipfsAddresses})
    } catch (error) {
        res.send({status: 201, msg: "Error", data: error})
    }
})

app.post('/', async (req, res) => {
    const b = req.body
    const address = await saveString.add(b.content.data)
    if(address){
        try{
            const ipfsData = await ipfsdata.create({
                type: req.body.type,
                CID: address,
                content: req.body.content
            })
            res.send({status: 400, msg: "Success", data: ipfsData})
        }catch(error){
            res.send({status: 201, msg: "Error", data: error})
        }
    }
})

app.post('/files', upload, async (req, res) => {

    const readableFileStream = fs.createReadStream("./fileUploads/file");
    const options = {
        pinataMetadata: {
        name: `file-${Date.now()}`
        },
    };

    const result = await pinata.pinFileToIPFS(readableFileStream, options);
    console.log(result.IpfsHash);

    if(result.IpfsHash){
        try{
            const ipfsData = await ipfsdata.create({
                type: "File",
                CID: result.IpfsHash,
                content: {
                "data": ""
                }
            })
            fs.unlink("./fileUploads/file", (err) => {
                if(err){
                    console.log(err)
                }
            })
            res.send({status: 400, msg: "Success", data: ipfsData})
        }catch(error){
            fs.unlink("./fileUploads/file", (err) => {
                if(err){
                    console.log(err)
                }
            })
            res.send({status: 201, msg: "Error", data: error})
        }
    }else{
        fs.unlink("./fileUploads/file", (err) => {
            if(err){
                console.log(err)
            }
        })
    }
})

app.listen(4000, () => {
    connectAndSubscribe()
    console.log("Server is listening 4000")
})

