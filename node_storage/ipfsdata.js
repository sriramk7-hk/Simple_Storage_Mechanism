import mongoose from "mongoose";

const { Schema } = mongoose

const contentSchema = new Schema({
    data: String
}, {timestamps: true})

const ipfsDataSchema = new Schema({
    type: String,
    CID: String,
    content: contentSchema
}, {timestamps: true})

export default mongoose.model('IpfsData', ipfsDataSchema)