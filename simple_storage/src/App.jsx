import axios from "axios"
import { Header, Container, Button, Grid, Table } from "semantic-ui-react"
import { Component } from "react";
import _ from "lodash"

class App extends Component {

  constructor(props){
    super(props);
    
    this.state = {
      files: [],
      file: {}
    }
    this.onReload()
  }


  handleFileChange = (event) => {
    this.setState({file: event.target.files[0]})
    console.log(event.target.files[0])
  }

  onReload = async () => {
    try {
      const res = await axios.get(`http://localhost:4000/`)
      this.setState({files : res.data.data})
    } catch (error) {
      console.log(error)
    }
  }

  handleSubmit = async () => {
    let {file} = this.state
    const form = new FormData()
    form.set("file", file)
    try {
      const res = await axios.post('http://localhost:4000/files',form)
      this.onReload()
    } catch (error) {
      console.log(error)
    }
  }

  render() {
    let {files, file} = this.state
    return (
      <Grid style={{margin: "20px"}}>
        <Grid.Row>
          <Header style={{paddingLeft: "35%"}}>IIoT data storage - BLOCKCHAIN</Header>
        </Grid.Row>
        <Grid.Row>
          <Grid.Column width="6" floated="left" style={{display: "grid", "rowGap": "4rem", width: "400px", padding: "80px", height: "460px"}}> 
              <Header textAlign="center">Upload Data</Header>
              <input hidden type="file" id="file" onChange={(event) => this.handleFileChange(event)}/>
              <label htmlFor="file" style={{border: "0.25rem dashed red", borderRadius: "0.5rem", height: "80px", textAlign: "center"}}>{ file.name ? <p>{`File uploaded ${file.name}`}</p> : <p style={{marginTop: "25px", fontWeight: "600"}}>Click to Upload Photo</p>}</label>
              <Button onClick={() => this.handleSubmit()}>Publish</Button>
          </Grid.Column>
          <Grid.Column floated="right" width="10" style={{ padding: "80px"}}>
            <Table celled striped>
              <Table.Header>
                <Table.Row>
                  <Table.HeaderCell>S.No</Table.HeaderCell>
                  <Table.HeaderCell>Content</Table.HeaderCell>
                  <Table.HeaderCell>Link</Table.HeaderCell>
                </Table.Row>
              </Table.Header>
              <Table.Body>
                {
                  files ? 
                    files.map((val, key) => (
                      <Table.Row key={key}>
                        <Table.Cell>{key+1}</Table.Cell>
                        <Table.Cell>{val.type}</Table.Cell>
                        <Table.Cell><a href={`https://ipfs.io/ipfs/${val.CID}`}>{val.CID}</a></Table.Cell>
                      </Table.Row>
                    )) : null
                }
              </Table.Body>
            </Table>
          </Grid.Column>
        </Grid.Row>
      </Grid>
    )
  }
}

export default App
