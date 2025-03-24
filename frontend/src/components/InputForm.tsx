import { useState } from "react";
import { Form, Button } from "react-bootstrap";

interface InputFormProps {
  onSubmit: (code: string) => void;
}

const InputForm: React.FC<InputFormProps> = ({ onSubmit }) => {
  const [inputCode, setInputCode] = useState("");

  const handleSubmit = () => {
    if (inputCode.trim() !== "") {
      onSubmit(inputCode);
      // setInputCode("");  <-- REMOVE this so text doesn't disappear
    } else {
      alert("Input cannot be empty!");
    }
  };

  return (
    <div className="mb-3">
      <Form.Group controlId="inputCode">
        <Form.Label>Enter Machine Code</Form.Label>
        <Form.Control
          as="textarea"
          rows={4}
          value={inputCode}
          onChange={(e) => setInputCode(e.target.value)}
        />
        <Button className="mt-2" onClick={handleSubmit}>
          Submit
        </Button>
      </Form.Group>
    </div>
  );
};

export default InputForm;
