import { Button } from "react-bootstrap";

// ✅ Fix: Ensure the `executionComplete` prop is properly defined
interface ControlsProps {
  onControl: (command: string) => void;
  executionComplete: boolean; 
}

const Controls: React.FC<ControlsProps> = ({ onControl, executionComplete }) => {
  return (
    <div className="mt-3">
      <Button
        variant="primary"
        className="me-2"
        onClick={() => onControl("next")}
        disabled={executionComplete} // Disable when execution is complete
      >
        Next
      </Button>
      <Button
        variant="success"
        className="me-2"
        onClick={() => onControl("run")}
        disabled={executionComplete} // Disable when execution is complete
      >
        Run
      </Button>
      <Button
        variant="danger"
        onClick={() => onControl("end")}
        disabled={executionComplete} // Disable when execution is complete
      >
        End
      </Button>
    </div>
  );
};

export default Controls;
