import { useEffect, useState } from "react";
import { Tabs, Tab, Row, Col, Button, Form, Container } from "react-bootstrap";
import "bootstrap/dist/css/bootstrap.min.css";
import MemoryTable from "./components/MemoryTable";

function App() {
  const [selectedTab, setSelectedTab] = useState("data");
  const [dataMemory, setDataMemory] = useState([]);
  const [stackMemory, setStackMemory] = useState([]);
  const [instructionMemory, setInstructionMemory] = useState([]);
  const [registers, setRegisters] = useState<{ id: number; value: number }[]>([]);
  const [executionComplete, setExecutionComplete] = useState(false);
  const [codeInput, setCodeInput] = useState("");
  const [isSubmitted, setIsSubmitted] = useState(false);
  const [logs, setLogs] = useState<string[]>([]);

  useEffect(() => {
    if (isSubmitted) {
      fetchMemory();
      fetchRegisters();
      checkExecutionStatus();
      fetchLogs();
    }
  }, [isSubmitted]);

  useEffect(() => {
    const interval = setInterval(fetchLogs, 500);
    return () => clearInterval(interval);
  }, []);

  const fetchMemory = async () => {
    const res = await fetch("http://localhost:5000/memory");
    const data = await res.json();
    setDataMemory(data.data || []);
    setStackMemory(data.stack || []);
    setInstructionMemory(data.instructions || []);
  };

  const fetchRegisters = async () => {
    try {
      const res = await fetch("http://localhost:5000/registers");
      const data = await res.json();

      if (data.registers && Array.isArray(data.registers)) {
        setRegisters([...data.registers]);
      } else {
        setRegisters(Array.from({ length: 32 }, (_, i) => ({ id: i, value: 0 })));
      }
    } catch (error) {
      console.error("[ERROR] Failed to fetch registers:", error);
      setRegisters(Array.from({ length: 32 }, (_, i) => ({ id: i, value: 0 })));
    }
  };

  const fetchLogs = async () => {
    try {
      const res = await fetch("http://localhost:5000/logs");
      const data = await res.json();
      setLogs(data.logs || []);
    } catch (error) {
      console.error("[ERROR] Failed to fetch logs:", error);
    }
  };

  const checkExecutionStatus = async () => {
    try {
      const res = await fetch("http://localhost:5000/execution-status");
      const data = await res.json();
      setExecutionComplete(data.executionComplete);
    } catch (error) {
      console.error("[ERROR] Failed to fetch execution status:", error);
    }
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setExecutionComplete(false);
    setIsSubmitted(false);

    await fetch("http://localhost:5000/submit", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ code: codeInput }),
    });

    setIsSubmitted(true);
    alert("New Code Submitted & Simulator Restarted!");
  };

  const handleControl = async (command: string) => {
    if (executionComplete) return;

    await fetch("http://localhost:5000/control", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ command }),
    });

    if (command === "N" || command === "E") {
        // ✅ If "Next" or "Exit", update immediately
              setTimeout(() => {
                  fetchMemory();
                  fetchRegisters();
                  checkExecutionStatus();
              }, 500);
          } else if (command === "R") {
              // ✅ If "Run", wait for execution to complete, then update once
              let checkInterval = setInterval(async () => {
                  const res = await fetch("http://localhost:5000/execution-status");
                  const data = await res.json();
                  if (data.executionComplete) {
                      clearInterval(checkInterval); // ✅ Stop checking once finished
                      fetchMemory();
                      fetchRegisters();
                      checkExecutionStatus();
                  }
              }, 500); // ✅ Check execution status every 500ms
          }
      };


  const formatLog = (log: string) => {
    if (log.includes("[Fetch]")) return <p style={{ color: "#00FFFF" }}> {log} </p>;
    if (log.includes("[Decode]")) return <p style={{ color: "#FFD700" }}> {log} </p>;
    if (log.includes("[Execute]")) return <p style={{ color: "#32CD32" }}> {log} </p>;
    if (log.includes("[WB]")) return <p style={{ color: "#FF4500" }}> {log} </p>;
    if (log.includes("Clock Cycle")) return <p style={{ fontWeight: "bold", color: "#FFFFFF" }}> {log} </p>;
    return <p style={{ color: "#B0B0B0" }}> {log} </p>;
  };

  return (
    <Container fluid className="mt-4">
      <h2 className="text-center">RISC-V Simulator</h2>

      <Row className="mb-3">
        {/* Left Side: Text Segment & Memory */}
        <Col md={7}>
          <Form onSubmit={handleSubmit}>
            <Form.Group>
              <Form.Control as="textarea" rows={10} value={codeInput} onChange={(e) => setCodeInput(e.target.value)} placeholder="Write your assembly code here..." />
            </Form.Group>
            <Button variant="primary" type="submit" className="mt-2 w-100">Submit Code</Button>
          </Form>

          <div className="d-flex justify-content-center gap-3 mt-3">
            <Button variant="success" onClick={() => handleControl("N")} disabled={!isSubmitted || executionComplete} size="lg">Next</Button>
            <Button variant="warning" onClick={() => handleControl("R")} disabled={!isSubmitted || executionComplete} size="lg">Run</Button>
            <Button variant="danger" onClick={() => handleControl("E")} disabled={!isSubmitted || executionComplete} size="lg">Exit</Button>
          </div>

          <Tabs activeKey={selectedTab} onSelect={(k) => setSelectedTab(k || "data")} className="mt-4">
            <Tab eventKey="data" title="Data Memory"><MemoryTable memory={dataMemory} /></Tab>
            <Tab eventKey="stack" title="Stack Memory"><MemoryTable memory={stackMemory} /></Tab>
            <Tab eventKey="instructions" title="Instruction Memory"><MemoryTable memory={instructionMemory} /></Tab>
          </Tabs>

          {/* Registers Below Memory */}
          <h5 className="mt-4">Registers</h5>
          <div className="d-grid gap-2">
            <table className="table table-bordered">
              <tbody>
                {Array.from({ length: 4 }, (_, row) => (
                  <tr key={row}>
                    {Array.from({ length: 8 }, (_, col) => {
                      const index = row * 8 + col;
                      const register = registers.find(r => r.id === index);
                      return (
                        <td key={index} className="text-center">
                          <b>R[{index}]</b><br />
                          {register ? register.value : 0}
                        </td>
                      );
                    })}
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </Col>

        {/* Right Side: WIDER Output Terminal */}
        <Col md={5}>
          <h5>Output Terminal</h5>
          <div className="terminal-box" style={{
            backgroundColor: "#1E1E1E",
            color: "#FFFFFF",
            padding: "10px",
            height: "500px",
            overflowY: "auto",
            borderRadius: "5px",
            border: "1px solid #333",
            fontFamily: "monospace",
            whiteSpace: "pre-wrap",
            width: "100%"
          }}>
            {logs.map((log, index) => (
              <div key={index}>{formatLog(log)}</div>
            ))}
          </div>
        </Col>
      </Row>
    </Container>
  );
}

export default App;
